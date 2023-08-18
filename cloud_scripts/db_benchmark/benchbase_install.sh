#!/bin/bash
#
# Cloning and building benchbase, according to https://github.com/cmu-db/benchbase/blob/main/README.md
#
git clone --depth 1 https://github.com/cmu-db/benchbase.git
cd benchbase
./mvnw clean package -P postgres
cd target
tar xvzf benchbase-postgres.tgz