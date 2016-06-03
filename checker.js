const fs = require('fs');

const args = process.argv.slice(2);

if (args.length < 1) {
  console.log('Error: A file directory must be specified');
  return;
}

const fileDirectory = args[0];

// XXX: This whole thing is in a mess, break them into different functions

fs.readdir(fileDirectory, (error, fileNames) => {
    if (error) {
      throw error;
      console.log(error);
    }

    fileNames.forEach((fileName) => {
      // imports holds the imports that we have
      let imports = [];
      // importUsed holds the true/false of whether the imports that we have have been used
      let importsUsed = [];

      if (fileName.substring(fileName.length - 1, fileName.length) === 'm' ||
          fileName.substring(fileName.length - 1, fileName.length) === 'h') {
            // NOTE: Skip directories for now, no recursion
            if (fs.lstatSync(`${fileDirectory}/${fileName}`).isDirectory() ||
                fileName.includes('PM') || fileName.includes('+')) {
              return;
            }

            fs.readFile(`${fileDirectory}/${fileName}`, 'utf8', (error, data) => {
              if (error) {
                throw error;
                console.log(error);
              }

              const fileLines = data;
              const splitLines = data.split('\n');

              for (line of splitLines) {
                if (line === '') {
                  continue;
                }

                // Temporary solution for MVP solution, ignore Macros, class extensions, imported frameworks and Swift header
                // NOTE: This is not the final amount of things to be excluded for this MVP
                if (line.includes('WMLMacros') || line.includes('+') || line.includes('-Swift') ||
                    line.includes('Gender') || (line.includes('<') && !line.includes('interface'))) {
                  continue;
                }

                if (line.includes('#import')) {
                  // Pick out the import name and add it into our imports array
                  const stringOpeningQuoteIndex = line.indexOf('"');
                  const importedHeader = line.substring(stringOpeningQuoteIndex + 1, line.length - 3);
                  imports.push(importedHeader);
                  // Make sure to also add a false value into our importsUsed array
                  importsUsed.push(false);
                } else {
                  // NOTE: This can be further optimized to first check whether or not it has already
                  // been noted to be used but let's not pursue the root of all evil yet
                  for (let i = 0; i < imports.length; i++) {
                    const importToCheck = imports[i];

                    if (line.includes(importToCheck)) {
                      importsUsed[i] = true;
                    }

                    continue;
                  }
                }
              }

              for (let i = 0; i < importsUsed.length; i++) {
                const isUsed = importsUsed[i];

                if (isUsed) {
                  continue;
                }

                const unusedImport = imports[i];

                // NOTE: Keep it here for now to do testing, uncomment when needed
                // console.log(`${unusedImport} is not used in this file`);

                // Simplest solution now, brute force - we will improve it in the future
                for (let i = 0; i < splitLines.length; i++) {
                  const fileLine = splitLines[i];

                  if (fileLine.includes(unusedImport)) {
                  // XXX: Really really terrible solution, to be improved
                    splitLines.splice(i, 1);
                    i--;
                  }
                }
              }

              let rebuiltFileLines = splitLines.join('\n');

              if (rebuiltFileLines[rebuiltFileLines.length]) {
                rebuiltFileLines.push('\n');
              }

              // NOTE: Keep it here for now to do testing, uncomment when needed
              // console.log(rebuiltFileLines);

              // TODO: Find a better way to write out test files
              // NOTE: To test, append other name to file so that it does not overwrite
              fs.writeFile(`${fileDirectory}/${fileName}`, rebuiltFileLines, (error) => {
                if (error) {
                  throw error;
                  console.log(error);
                }
              });
            });
          }
    });

    console.log('Writing of files completed');
});
