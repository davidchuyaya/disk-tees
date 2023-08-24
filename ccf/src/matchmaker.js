/**
 * Test the API with the following commands, after executing ccf_launch.sh and navigating to workspace/sandbox_common:
 * 
 * 1. Check addcert and getcert. getcert should return "blah"
 * curl https://127.0.0.1:8000/matchmaker/addcert -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"id": 42, "cert": "blah"}'
 * curl https://127.0.0.1:8000/matchmaker/getcert -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"id": 42}'
 * 
 * 2. Check match
 * curl https://127.0.0.1:8000/matchmaker/match -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"round": "1,0,1", "config": "config1"}'
 * 2.1 config1 should be returned in the next call, with round 1,0,2
 * curl https://127.0.0.1:8000/matchmaker/match -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"round": "1,0,2", "config": "config2"}'
 * 2.2 no config should be returned in the next call, with round 1,0,2
 * curl https://127.0.0.1:8000/matchmaker/match -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"round": "1,0,0", "config": "config0"}'
 * 
 * 3. Check garbage. Config 1 should be deleted.
 * curl https://127.0.0.1:8000/matchmaker/match -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"round": "1,0,3", "config": "config3"}'
 * curl https://127.0.0.1:8000/matchmaker/garbage -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"round": "1,0,3"}'

 */
/*
  Prefix with "public:" because data doesn't need to be encrypted.
  See here: https://microsoft.github.io/CCF/main/build_apps/kv/kv_how_to.html#map-naming
*/
const keyCerts = "public:certs";
const keyConfigs = "public:configs";
const keyRound = "public:round";
const keyRoundHighest = "highestRound";

function get_round_from_query(parsedQuery) {
  if (parsedQuery.round === undefined) {
    throw new Error("Could not find 'round' in query");
  }
  const elements = parsedQuery.round.split(",");
  if (elements.length !== 3) {
    throw new Error(`Invalid round format: ${parsedQuery.round}`);
  }
  return elements.map((e) => parseInt(e));
}

// String format is 0,0,0
function round_to_string(round) {
  return round.map((e) => e.toString()).join(",");
}

function round_to_buf(round) {
  return ccf.strToBuf(round_to_string(round));
}

function round_from_buf(buf) {
  const elements = ccf.bufToStr(buf).split(",");
  return elements.map((e) => parseInt(e));
}

function get_round_from_ccf() {
  const roundBuf = ccf.kv[keyRound].get(ccf.strToBuf(keyRoundHighest));
  if (roundBuf === undefined) {
    return [0,0,0];
  }
  return round_from_buf(roundBuf);
}

function set_round_to_ccf(round) {
  ccf.kv[keyRound].set(ccf.strToBuf(keyRoundHighest), round_to_buf(round));
}

function round_less_than(round1, round2) {
  return round1[0] < round2[0] ||
    (round1[0] === round2[0] && round1[1] < round2[1]) ||
    (round1[0] === round2[0] && round1[1] === round2[1] && round1[2] < round2[2]);
}

/*
  Endpoints
 */

/**
 * Adds a certificate for a replica, if one does not yet exist.
 * Overwriting a certificate can introduce security issues.
 * 
 * @param {id, cert} request id of the replica, public certificate of the replica
 * @returns {body: true} if the certificate was added successfully
 */
export function on_add_cert(request) {
  const parsedQuery = request.body.json();
  const id = parsedQuery.id;
  const cert = parsedQuery.cert;
  if (id === undefined || cert === undefined) {
    return { body: { error: `Missing id ${id} or cert ${cert}` } };
  }
  if (ccf.kv[keyCerts].has(ccf.strToBuf(id.toString()))) {
    return { body: { error: `Cert already exists for id: ${id}` } };
  }

  ccf.kv[keyCerts].set(ccf.strToBuf(id.toString()), ccf.strToBuf(cert));
  return { body: true };
}

/**
 * Gets the certificate for a replica, if one exists.
 * 
 * @param {id} request id of the replica
 * @returns {body: cert} if the certificate has been added
 */
export function on_get_cert(request) {
  const parsedQuery = request.body.json();
  const id = parsedQuery.id;
  if (id === undefined) {
    return { body: { error: `Missing id ${id}` } };
  }
  const cert = ccf.kv[keyCerts].get(ccf.strToBuf(id.toString()));
  if (cert === undefined) {
    return { body: { error: `No cert for id: ${parsedQuery.id}` } };
  }
  return { body: ccf.bufToStr(cert) };
}

/**
 * If the client's round is higher, set the config, and return all previous configs.
 * If the client's round is lower, return the highest round.
 * 
 * @param {round, config} request The client's round formatted as "0,0,0", config is a string 
 * @returns {configs, round} configs is an array of configs, round is the highest round in the matchmaker
 */
export function on_match_a(request) {
  const parsedQuery = request.body.json();
  let round;
  try {
    round = get_round_from_query(parsedQuery);
  }
  catch (e) {
    return { body: { error: e.message } };
  }
  const highestRound = get_round_from_ccf();
  // if the client's round is higher
  if (round_less_than(highestRound, round)) {
    set_round_to_ccf(round);
    ccf.kv[keyConfigs].set(round_to_buf(round), ccf.strToBuf(parsedQuery.config));
    
    // return all previous configs
    const configs = [];
    ccf.kv[keyConfigs].forEach(function (config, configRound) {
      if (round_less_than(round_from_buf(configRound), round)) {
        configs.push(ccf.bufToStr(config));
      }
    });
    return { body: {
      configs: configs,
      round: round_to_string(round)
    } };
  }

  // if the client's round is lower
  return { body: {
    configs: [],
    round: round_to_string(highestRound)
  } };
}

/**
 * Delete all configs with a round less than the client's round.
 * 
 * @param {round} request The client's round formatted as "0,0,0"
 * @returns {body: true} if the garbage collection was successful
 */
export function on_garbage_a(request) {
  const parsedQuery = request.body.json();
  const round = get_round_from_query(parsedQuery);
  ccf.kv[keyConfigs].forEach(function (config, configRound) {
    if (round_less_than(round_from_buf(configRound), round)) {
      ccf.kv[keyConfigs].delete(configRound); 
    }
  });

  return { body: true };
}