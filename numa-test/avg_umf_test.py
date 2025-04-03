import subprocess
import re

def run_program_and_average(prog_path, x, n):
    """
    Runs the specified program `n` times with input `x`, extracts the elapsed time from the output,
    and computes the average elapsed time in nanoseconds and seconds.
    
    :param prog_path: Path to the program to execute
    :param x: Input argument for the program
    :param n: Number of times to run the program
    :return: None
    """
    elapsed_ns = []
    elapsed_s = []
    
    # Regex patterns to extract elapsed time in ns and s
    ns_pattern = r"Elapsed \(ns\): (\d+)"
    s_pattern = r"Elapsed \(s\): ([0-9]*\.?[0-9]+)"
    
    for i in range(n):
        try:
            # Run the program with argument `x`
            result = subprocess.run([prog_path, str(x)], capture_output=True, text=True, check=True)
            output = result.stdout
            
            # Extract elapsed times using regex
            ns_match = re.search(ns_pattern, output)
            s_match = re.search(s_pattern, output)
            
            if ns_match and s_match:
                elapsed_ns.append(int(ns_match.group(1)))
                elapsed_s.append(float(s_match.group(1)))
            else:
                print(f"Run {i+1}: Unable to parse elapsed times from output.")
        except Exception as e:
            print(f"Run {i+1}: Error occurred - {e}")
    
    # Compute averages
    if elapsed_ns and elapsed_s:
        avg_ns = sum(elapsed_ns) / len(elapsed_ns)
        avg_s = sum(elapsed_s) / len(elapsed_s)
        print(f"Average Elapsed Time: {avg_ns:.2f} ns ({avg_s:.6f} s)")
    else:
        print("No valid data to compute averages.")

# Example usage
if __name__ == "__main__":
    prog_path = "./bin/umf_test"  # Path to the program
    x = input("Enter value for x: ")  # User input for x
    n = int(input("Enter number of runs (n): "))  # User input for n
    run_program_and_average(prog_path, x, n)
