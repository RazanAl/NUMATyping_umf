import csv
import subprocess

#!/usr/bin/python


# Copyright 2015 University of Rochester
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License. 




import sys 
import shutil
import csv
from argparse import ArgumentParser
from subprocess import call
import os

def isInt(s):
	try:
		int(s)
		return True
	except ValueError:
		return False

def parseCommandLine():
	parser = ArgumentParser("metacmd.py CMD OPTION1 OPTION2 [--meta METAOPTION:VALUE1:VALUE2:INT1...INT2] [-h] [--printOnly]\
\nMetacmd will in invoke CMD on all combinations of meta options and their values.  Non meta options will be passed as is to the command.")	
	parser.add_argument("--meta", action="append",dest="metas",help="meta options to iterate over. Flag comes first in colon separated list.  Can use ellipses in between integers to represent all integers in the range.", metavar="OPTION:VALUE1,VALUE2,VALUE3", default = [])
	parser.add_argument("--printOnly", action="store_true",dest="printOnly",help="show commands, but don't actually execute them", default = False)
	parser.add_argument("--csv_file", type=str, help="Name of the CSV file")
	parser.add_argument("--repeat", type=int, help="number of times each exp is repeated")

	(options, remains) = parser.parse_known_args()

	# print ("Metas "), options.metas
	# print ("Remaining arguments (passed to command) "), remains

	# print("Date, Time, DS_name, num_DS, num_threads, thread_config, DS_config, duration, Op0, Op1, TotalOps")
	for meta in options.metas:
		idx = meta.find(":")
		if(idx==-1):
			print ("Error on: "), meta
			sys.exit("Every meta-option must have colon to separate option from values.")
		if(idx==len(meta)-1):
			print ("Error on: "), meta
			sys.exit("Meta-option cannot end with colon.")

	return options, remains

def parseMetaOption(meta):
	idx = meta.find(":")
	opt = meta[:idx]
	if(idx==0):
		opt = ""
	if(len(opt)==1):
		opt = "-"+opt
	if(len(opt)>=1 and opt.find("-")==-1):
		opt = "--"+opt
	return opt

def parseMetaValues(meta):
	idx = meta.find(":")
	vals = meta[idx+1:]
	vals = vals.split(":")
	nextVals = []
	for val in vals:
		eidx = val.find("...")
		if(eidx!=-1):
			if(eidx==len(val)-3):
				print ("Error on: "), meta
				sys.exit("Ellipses(...) cannot end metavalue list.")
			if(eidx==0):
				print ("Error on: "), meta
				sys.exit("Ellipses(...) cannot start metavalue list.")
			before = val[:eidx]
			after = val[eidx+3:]
			if(isInt(before) and isInt(after)):
				for i in range(int(before),int(after)+1):
					nextVals.append(str(i))
			else:
				print ("Error on: "), meta
				sys.exit("Ellipses(...) must be in between two integers.")
		else:
			nextVals.append(val)
	return nextVals


# CSV_FILE = "output_results.csv"  # Name of the CSV file

if __name__ == "__main__":
	options, remains = parseCommandLine()
	metaDict = {}
	CSV_FILE = options.csv_file
	repetition = options.repeat
	# Parse meta options
	for meta in options.metas:
		opt = parseMetaOption(meta)
		vals = parseMetaValues(meta)
		metaDict[opt] = vals if opt not in metaDict else metaDict[opt] + vals

	# Generate all combinations
	combos = [""]
	nextCombos = []
	for opt, vals in metaDict.items():
		for s in combos:
			for v in vals:
				nextCombos.append(f"{s} {v}")
		combos = nextCombos
		nextCombos = []

	# Prepare CSV file
	with open(CSV_FILE, "w", newline="") as csvfile:
		csv_writer = csv.writer(csvfile)
		header_row = ["Allocator", "num_threads"] + [f"Time(s)_{i+1}" for i in range(repetition)] + ["Average"]
		csv_writer.writerow(header_row)  # CSV header
		print

		for combo in combos:
			cmd_parts = combo.strip().split()  # Extract individual arguments
			if len(cmd_parts) < 2:
				print(f"Skipping invalid combination: {combo}")
				continue

			arg1, arg2 = cmd_parts[:2]  # Extract first two arguments
			
			cmd = " ".join(remains) + f" {arg1} {arg2}"
			
			if options.printOnly:
				print(f"(Dry Run) {cmd}")
			else:
				try:
					result = []
					for i in range(repetition):
						process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
						stdout, stderr = process.communicate()
						# Get last non-empty line from output
						last_line = stdout.strip().split("\n")[-1] if stdout.strip() else "(No output)"
						result.append(float(last_line))
					average = sum(result)/ len(result) if result else 0
					# Write to CSV
					csv_writer.writerow([arg1, arg2] + result + [f"{average}"])
					print(f"Executed: {cmd}")
				except Exception as e:
					print(f"Error running command '{cmd}': {e}")
