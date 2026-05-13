#pragma once
#include <QObject>
#include <QTranslator>

class TranslationManager : public QObject {
    Q_OBJECT

public:
    static TranslationManager& instance();
    void setLanguage(const QString& lang);

signals:
    void languageChanged();

private:
    TranslationManager() = default;
    QTranslator m_translator;
};