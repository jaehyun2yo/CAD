#pragma once

#include "dxf_reader.h"
#include <QThread>
#include <QObject>
#include <QString>

namespace yjcad {

/// Background worker for loading DXF files without blocking UI
class DxfLoadWorker : public QObject {
    Q_OBJECT

public:
    explicit DxfLoadWorker(Document& doc, const QString& filePath);

signals:
    void progress(int percent, const QString& message);
    void finished(bool success, const QString& message, int entityCount, int layerCount);
    void error(const QString& message);

public slots:
    void process();

private:
    Document& m_doc;
    QString m_filePath;
};

/// Convenience wrapper: spawns a QThread + DxfLoadWorker
class DxfLoadTask : public QObject {
    Q_OBJECT

public:
    DxfLoadTask(Document& doc, const QString& filePath, QObject* parent = nullptr);
    ~DxfLoadTask() override;

    void start();

signals:
    void progress(int percent, const QString& message);
    void finished(bool success, const QString& message, int entityCount, int layerCount);

private:
    QThread m_thread;
    DxfLoadWorker* m_worker;
};

} // namespace yjcad
