#include <NifFile.hpp>

#include "PreviewNif.h"
#include "NifWidget.h"

#include <QGridLayout>
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
    return MOBase::VersionInfo(0, 1, 5, MOBase::VersionInfo::RELEASE_BETA);
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
    auto path = std::filesystem::path(fileName.toStdWString());
    auto nifFile = std::make_shared<nifly::NifFile>(path);

    if (!nifFile->IsValid()) {
        qWarning(qUtf8Printable(tr("Failed to load file: %1").arg(fileName)));
        return nullptr;
    }

    auto layout = new QGridLayout();
    layout->setRowStretch(0, 1);
    layout->setColumnStretch(0, 1);

    layout->addWidget(makeLabel(nifFile.get()), 1, 0, 1, 1);

    auto nifWidget = new NifWidget(nifFile, m_MOInfo);
    layout->addWidget(nifWidget, 0, 0, 1, 1);

    auto widget = new QWidget();
    widget->setLayout(layout);
    return widget;
}

QLabel* PreviewNif::makeLabel(nifly::NifFile* nifFile) const
{
    int shapes = 0;
    int faces = 0;
    int verts = 0;

    for (auto& shape : nifFile->GetShapes()) {
        shapes++;
        faces += shape->GetNumTriangles();
        verts += shape->GetNumVertices();
    }

    auto text = tr("Verts: %1 | Faces: %2 | Shapes: %3").arg(verts).arg(faces).arg(shapes);
    auto label = new QLabel(text);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}
