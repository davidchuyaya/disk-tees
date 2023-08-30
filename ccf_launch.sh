#!/bin/bash
# Documentation here: https://github.com/microsoft/CCF/blob/main/tests/sandbox/sandbox.sh
# and here: https://github.com/microsoft/CCF/blob/main/tests/start_network.py
/opt/ccf_virtual/bin/sandbox.sh --verbose \
    --package /opt/ccf_virtual/lib/libjs_generic \
    --js-app-bundle ~/disk-tees/ccf
    # --enclave-type release \
    # --node \ # TODO: List of (local://|ssh://)hostname:port[,pub_hostnames:pub_port]. Default is {DEFAULT_NODES}
# https://microsoft.github.io/CCF/main/build_apps/run_app.html#authentication
# TODO: Copy keys from workspace/sandbox_common/user0_cert.pem, user0_privk.pem, and service_cert.pem and give them to the client