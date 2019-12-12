TEMPLATE = subdirs

qtHaveModule(quick):qtHaveModule(waylandcompositor) {
    SUBDIRS += \
        compositor
}
