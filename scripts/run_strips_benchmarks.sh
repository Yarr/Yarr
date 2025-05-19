TEST_DATA_DIR=/tmp/test_strips_data

mkdir -p ${TEST_DATA_DIR}

for TYPE in basic empty full empty_counters full_counters
do
  echo "Running benchmark for packet type: ${TYPE}"
  TEST_FILE=${TEST_DATA_DIR}/data_file_${TYPE}.bin
  python3 ./scripts/make_strips_data.py -t ${TYPE} -o ${TEST_FILE}
  ./bin/benchmark_data_processor_star -f ${TEST_FILE}
done
