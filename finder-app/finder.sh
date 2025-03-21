#!/bin/sh

if [ $# -ne 2 ]; then
	echo -e "Too few arguments were submitted.\nExample:\n\t./find.sh /home/user/dir 'my search string'"
	exit 1
fi

filesdir="$1"
searchstr="$2"

if [ ! -d ${filesdir} ]; then
	echo "The directory '${filesdir}' doesn't exist..."
	exit 1
fi

# Count the number of files in filesdir
number_of_files=$(find ${filesdir} -type f | wc -l)
# Count the occurrences of searchstr in found files
matching_lines=$(grep -r ${searchstr} ${filesdir} | wc -l)

echo "The number of files are ${number_of_files} and the number of matching lines are ${matching_lines}"