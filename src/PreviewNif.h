#pragma once

#include <ipluginpreview.h>

class PreviewNif : public MOBase::IPluginPreview
{
    Q_OBJECT
    Q_INTERFACES(MOBase::IPlugin MOBase::IPluginPreview)
    Q_PLUGIN_METADATA(IID "org.tannin.PreviewNif" FILE "previewnif.json")

public:
    PreviewNif() = default;

    // IPlugin Interface

    bool init(MOBase::IOrganizer* moInfo) override;
    QString name() const override;
    QString author() const override;
    QString description() const override;
    MOBase::VersionInfo version() const override;
    QList<MOBase::PluginSetting> settings() const override;
    bool enabledByDefault() const override;

    // IPluginPreview interface

    std::set<QString> supportedExtensions() const override;
    QWidget* genFilePreview(const QString& fileName, const QSize& maxSize) const override;

private:
    MOBase::IOrganizer* m_MOInfo;
};
