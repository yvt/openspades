# Troubleshooting

## Check to see if the issue has been reported

***Before* creating an issue, make sure you've searched on the [Issue Tracker](https://github.com/yvt/openspades/issues?utf8=%E2%9C%93&q=is%3Aissue%20) to see if someone else has already reported the same issue**. Use relevant keywords.


## Common Issues

This is a list of commonly encountered problems, known issues, and their solutions.

### Failed to dlload 'libopenal.so'

Make sure libopenal is actually installed.

If it is installed, you need to copy and rename the file to to *libopenal.so*.

For example:
`sudo cp /usr/lib/x86_64-linux-gnu/libopenal.so.1 /usr/lib/x86_64-linux-gnu/libopenal.so`


## Create an issue

If your problem hasn't been solved or reported, then create an issue detailing your problem in as much detail as possible.
