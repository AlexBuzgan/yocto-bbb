SUMMARY = "Led Control Program"
DESCRIPTION = "Uses GPIO sfsys"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://ledcontrol.c"

S = "${WORKDIR}/build"

do_compile(){
    ${CC} ${CFLAGS} ${LDFLAGS} ${WORKDIR}/ledcontrol.c -o ${S}/ledcontrol
}

do_install(){
    install -d ${D}${bindir}
    install -m 0755 ${S}/ledcontrol ${D}${bindir}/
}
