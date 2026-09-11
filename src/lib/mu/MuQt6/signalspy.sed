# NOTE: these are applied as literal string replacements by
# cmake/scripts/apply_sed_filter.cmake, not by sed. Regular expressions and
# character classes will NOT match, so keep the patterns literal.
#
# SignalSpy derives from QSignalSpy up to Qt 6.5 and from QObject as of Qt 6.8
# (see SignalSpy.h), so the base call moc generates differs. Both forms are
# listed; the one that does not apply is simply a no-op.
s/::SignalSpy::qt_metacall/::SignalSpy::original_qt_metacall/
s/_id = QSignalSpy::qt_metacall(_c, _id, _a);//
s/_id = QObject::qt_metacall(_c, _id, _a);//
