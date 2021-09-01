# Data branch
This branch contains binary tools and files that are downloaded and used by the
main tool. Complete list of files you can see in the
[manifest file](manifest.xml).

## apm-jni
Name of each JAR file corresponds to the APM version. They are contain helper
Java classes which are used via JNI. Source code is located
[here](https://github.com/lem0nez/apm/tree/main/java).

## assets
This folder contains files required to build applications.
`project-template.zip` is base for all projects and the `debug.jks` Java
KeyStore is used to sign applications for debugging.

## tools
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
They are include `posixrules` timezone, that is required by `aapt2`.
