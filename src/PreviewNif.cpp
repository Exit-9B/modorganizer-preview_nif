#include <NifFile.hpp>

#include "PreviewNif.h"
#include "NifWidget.h"

#include <filesystem>

bool PreviewNif::init(MOBase::IOrganizer* moInfo)
{
    m_MOInfo = moInfo;
    return true;
}

QString PreviewNif::name() const
{
    return "Preview NIF";
}

QString PreviewNif::author() const
{
    return "Parapets";
}

QString PreviewNif::description() const
{
    return "Supports previewing NIF files";
}

MOBase::VersionInfo PreviewNif::version() const
{
    return MOBase::VersionInfo(0, 0, 0, MOBase::VersionInfo::RELEASE_ALPHA);
}

QList<MOBase::PluginSetting> PreviewNif::settings() const
{
    return QList<MOBase::PluginSetting>();
}

bool PreviewNif::enabledByDefault() const
{
    return true;
}

std::set<QString> PreviewNif::supportedExtensions() const
{
    return { "nif" };
}

QWidget* PreviewNif::genFilePreview(const QString& fileName, const QSize& maxSize) const
{
    auto path = std::filesystem::path(fileName.toStdString());
    auto nifFile = std::make_shared<nifly::NifFile>(path);

    if (!nifFile->IsValid()) {
        qWarning(qUtf8Printable(tr("Failed to load file: %1").arg(fileName)));
        return nullptr;
    }

    return new NifWidget(nifFile, m_MOInfo, true);
}
