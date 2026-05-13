#include "TranslationManager.h"
#include <QApplication>

TranslationManager& TranslationManager::instance() {
    static TranslationManager inst;
    return inst;
}

void TranslationManager::setLanguage(const QString& lang) {
    qApp->removeTranslator(&m_translator);

    if (m_translator.load(":/translations/Asset/translations/shuttle_" + lang + ".qm")) {
        qApp->installTranslator(&m_translator);
        emit languageChanged();
    }
}