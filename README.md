# Data branch
This branch contains binary tools and files that are downloaded and used by the
main tool. Complete list of files are contained in the
[manifest file](manifest.xml).

## Tools
Tools compiled with help of the
[Android tools builder](https://github.com/lem0nez/android-tools-builder)
project, without which they are wouldn't be available on so wide variety of
architectures.

### aapt2
Used to compile and package APK's resources. For this tool was applied a patch
that changes a path of the dependent `tzdata` file, that is downloaded by APM
from this branch.

### zipalign
Aligns the resulting APK file to reduce memory usage by an application.

## tzdata
[Timezone database](https://www.iana.org/time-zones) files that built to Android
supported format using the
[meefik's script](https://github.com/meefik/tzupdater/blob/master/app/src/main/assets/all/bin/tzdata-updater.sh).
They are include `posixrules` timezone, that required by `aapt2`.
