#!/bin/bash
unset IDF_PYTHON_ENV_PATH
./esp-idf/install.sh
. ./esp-idf/export.sh
idf.py fullclean