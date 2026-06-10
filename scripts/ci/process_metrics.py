# Process output expected by job_test_benchmark and
# generate report in metrics.txt
# See:
#   https://docs.gitlab.com/ci/testing/metrics_reports/
#   https://prometheus.io/docs/instrumenting/exposition_formats/#openmetrics-text-format

def is_hyphen_line(l):
    return l == ("-" * 79)

def is_eq_line(l):
    return l == ("=" * 79)

def is_dot_line(l):
    return l == ("." * 79)

def iter_until(ii, pred):
    for line in ii:
        if pred(line):
            return
        yield line

def parse_test_section(lines):
    yield "Test file name", list(iter_until(lines, is_dot_line))
    yield "Test header", list(iter_until(lines, is_hyphen_line))
    yield "Test results", list(iter_until(lines, is_hyphen_line))

def catch_block_stream(lines):
    """
    Process lines from iterator and return blocks.
    """
    yield "Header", list(iter_until(lines, is_hyphen_line))
    for line in lines:
        yield "Test name", [line]
        yield from parse_test_section(lines)

def print_test_lines(test_name, results_block):
    test_case_name = None
    for line in results_block:
        if len(line) > 1:
            first = line[0]
            if first == '=':
                break
            if line[0] != " " and not line[0].isdigit():
                test_case_name = line.split()[0]
                continue
        if test_case_name:
            mean_split = line.split()
            print(f"bench_{test_name}_{test_case_name} {mean_split[0]}")
            # Don't process following line
            test_case_name = None

def process_catch_bench(fname):
    """
    Process results from file generated via catch2
    """
    f = open(fname)

    lines = (line.strip() for line in f)
    test_name = None

    for t, block in catch_block_stream(lines):
        if t == "Test name":
            test_name = block[0]
            continue
        if t == "Test results":
            print_test_lines(test_name, block)

def process_logging_bench(fname, prefix):
    """
    Process data from star data processor benchmark.

    Strips out the part with a known logger name.
    """
    for l in open(fname):
        if l.startswith("Running"):
            test_packet_type = l.split(":")[1]
            test_packet_type = test_packet_type.strip()
            continue

        # Logger name used to output info
        if "benchmark_dataprocessing_star" not in l:
            continue

        if "Throughput" in l:
            info_split = l.split()
            bench_type = info_split[-3]
            number = info_split[-1]
            print(f"bench_{prefix}_{test_packet_type}_{bench_type} {number}")

def main():
    process_catch_bench("testUtils.benchmark")
    process_catch_bench("testYarr.benchmark")

    process_logging_bench("star_processor_benchmarks.benchmark",
                          "star_dataprocessor")

if __name__ == "__main__":
    main()

main()
