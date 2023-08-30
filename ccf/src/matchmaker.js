/**
 * Test the API with the following commands, after executing ccf_launch.sh and navigating to workspace/sandbox_common:
 * 
 * 1. Check addcert and getcert. getcert should return "blah"
 * curl https://127.0.0.1:8000/matchmaker/addcert -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"id": 42, "cert": "blah", "type": "client"}'
 * curl https://127.0.0.1:8000/matchmaker/getcert -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"id": 42, "type": "client"}'
 * 
 * 2. Check match
 * curl https://127.0.0.1:8000/matchmaker/match -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"ballot": "1,0,1", "config": "config1"}'
 * 2.1 config1 should be returned in the next call, with ballot 1,0,2
 * curl https://127.0.0.1:8000/matchmaker/match -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"ballot": "1,0,2", "config": "config2"}'
 * 2.2 no config should be returned in the next call, with ballot 1,0,2
 * curl https://127.0.0.1:8000/matchmaker/match -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"ballot": "1,0,0", "config": "config0"}'
 * 
 * 3. Check garbage. Config 1 should be deleted.
 * curl https://127.0.0.1:8000/matchmaker/match -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"ballot": "1,0,3", "config": "config3"}'
 * curl https://127.0.0.1:8000/matchmaker/garbage -X POST --cacert service_cert.pem --cert user0_cert.pem --key user0_privk.pem -H "Content-Type: application/json" --data-binary '{"ballot": "1,0,3"}'

 */
/*
  Prefix with "public:" because data doesn't need to be encrypted.
  See here: https://microsoft.github.io/CCF/main/build_apps/kv/kv_how_to.html#map-naming
*/
const keyClientCerts = "public:clientCerts";
const keyReplicaCerts = "public:replicaCerts";
const keyConfigs = "public:configs";
const keyBallot = "public:ballot";
const keyBallotHighest = "highestBallot";

function get_cert_table_from_type(query) {
  if (parsedQuery.type === undefined) {
    throw new Error("Could not find 'type' in query");
  }
  switch (parsedQuery.type) {
    case "client":
      return keyClientCerts;
    case "replica":
      return keyReplicaCerts;
    default:
      throw new Error(`Invalid type: ${parsedQuery.type}`);
  }
}

function get_ballot_from_query(parsedQuery) {
  if (parsedQuery.ballot === undefined) {
    throw new Error("Could not find 'ballot' in query");
  }
  const elements = parsedQuery.ballot.split(",");
  if (elements.length !== 3) {
    throw new Error(`Invalid ballot format: ${parsedQuery.ballot}`);
  }
  return elements.map((e) => parseInt(e));
}

// String format is 0,0,0
function ballot_to_string(ballot) {
  return ballot.map((e) => e.toString()).join(",");
}

function ballot_to_buf(ballot) {
  return ccf.strToBuf(ballot_to_string(ballot));
}

function ballot_from_buf(buf) {
  const elements = ccf.bufToStr(buf).split(",");
  return elements.map((e) => parseInt(e));
}

function get_ballot_from_ccf() {
  const ballotBuf = ccf.kv[keyBallot].get(ccf.strToBuf(keyBallotHighest));
  if (ballotBuf === undefined) {
    return [0,0,0];
  }
  return ballot_from_buf(ballotBuf);
}

function set_ballot_to_ccf(ballot) {
  ccf.kv[keyBallot].set(ccf.strToBuf(keyBallotHighest), ballot_to_buf(ballot));
}

function ballot_less_than(ballot1, ballot2) {
  return ballot1[0] < ballot2[0] ||
    (ballot1[0] === ballot2[0] && ballot1[1] < ballot2[1]) ||
    (ballot1[0] === ballot2[0] && ballot1[1] === ballot2[1] && ballot1[2] < ballot2[2]);
}

/*
  Endpoints
 */

/**
 * Adds a certificate for a node, if one does not yet exist.
 * Overwriting a certificate can introduce security issues.
 * 
 * @param {id, cert, type} request Node id, node public certificate, type = "replica" or "client"
 * @returns {body: true} if the certificate was added successfully
 */
export function on_add_cert(request) {
  const parsedQuery = request.body.json();
  const id = parsedQuery.id;
  const cert = parsedQuery.cert;
  const certTable = get_cert_table_from_type(parsedQuery);
  if (id === undefined || cert === undefined) {
    return { body: { success: false, error: `Missing id ${id} or cert ${cert}` } };
  }
  if (ccf.kv[certTable].has(ccf.strToBuf(id.toString()))) {
    return { body: { success: false, error: `Cert already exists for id: ${id}` } };
  }

  ccf.kv[certTable].set(ccf.strToBuf(id.toString()), ccf.strToBuf(cert));
  return { body: { success: true } };
}

/**
 * Gets the certificate for a node, if one exists.
 * 
 * @param {id} request Node id, type = "replica" or "client"
 * @returns {body: cert} if the certificate has been added
 */
export function on_get_cert(request) {
  const parsedQuery = request.body.json();
  const id = parsedQuery.id;
  const certTable = get_cert_table_from_type(parsedQuery);
  if (id === undefined) {
    return { body: { success: false, error: `Missing id ${id}` } };
  }
  const cert = ccf.kv[certTable].get(ccf.strToBuf(id.toString()));
  if (cert === undefined) {
    return { body: { success: false, error: `No cert for id: ${parsedQuery.id}` } };
  }
  return { body: { success: true, cert: ccf.bufToStr(cert) } };
}

/**
 * If the client's ballot is higher, set the config, and return all previous configs.
 * If the client's ballot is lower, return the highest ballot.
 * 
 * @param {ballot, config} request The client's ballot formatted as "0,0,0", config is a JSON object
 * @returns {configs, ballot} configs is an array of configs, ballot is the highest ballot in the matchmaker
 */
export function on_match_a(request) {
  const parsedQuery = request.body.json();
  let ballot = get_ballot_from_query(parsedQuery);
  const highestBallot = get_ballot_from_ccf();
  // if the client's ballot is higher
  if (ballot_less_than(highestBallot, ballot)) {
    set_ballot_to_ccf(ballot);
    ccf.kv[keyConfigs].set(ballot_to_buf(ballot), ccf.strToBuf(JSON.stringify(parsedQuery.config)));
    
    // return all previous configs
    const configs = [];
    ccf.kv[keyConfigs].forEach(function (config, configBallot) {
      if (ballot_less_than(ballot_from_buf(configBallot), ballot)) {
        configs.push(ccf.bufToStr(config));
      }
    });
    return { body: {
      success: true,
      configs: configs,
      ballot: ballot_to_string(ballot)
    } };
  }

  // if the client's ballot is lower
  return { body: {
    success: true,
    configs: [],
    ballot: ballot_to_string(highestBallot)
  } };
}

/**
 * Delete all configs with a ballot less than the client's ballot.
 * 
 * @param {ballot} request The client's ballot formatted as "0,0,0"
 * @returns {body: true} if the garbage collection was successful
 */
export function on_garbage_a(request) {
  const parsedQuery = request.body.json();
  const ballot = get_ballot_from_query(parsedQuery);
  ccf.kv[keyConfigs].forEach(function (config, configBallot) {
    if (ballot_less_than(ballot_from_buf(configBallot), ballot)) {
      ccf.kv[keyConfigs].delete(configBallot); 
    }
  });

  return { body: { success: true } };
}