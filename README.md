# biggestWord

Given a dictionary, find the longest word which does not contain any other word.
Word one contains word two if word two can be constructed from word one by deleting
some characters and permuting the rest. For fun.

## CLI
biggestWord accepts the path of the wordlist to use, and supports the following flags
- (-u/--url) download from url with libcurl
- (-m/--min_size) integer
