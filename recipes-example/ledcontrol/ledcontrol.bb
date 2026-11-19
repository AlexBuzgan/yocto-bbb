SUMMARY = "Led Control Program"
DESCRIPTION = "Uses GPIO sfsys"
LICENSE = "MIT"

LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://ledcontrol.c \
          file://ledctrl.service"

S = "${WORKDIR}/build"

DEPENDS = "libgpiod"

inherit systemd

SYSTEMD_AUTO_ENABLE = "enable"
SYSTEMD_SERVICE:${PN} = "ledctrl.service"

do_compile(){
    ${CC} ${CFLAGS} ${LDFLAGS} ${WORKDIR}/ledcontrol.c -o ${S}/ledcontrol -lgpiod
}

do_install(){
    install -d ${D}${bindir}
    install -m 0755 ${S}/ledcontrol ${D}${bindir}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/ledctrl.service ${D}${systemd_system_unitdir}
}
