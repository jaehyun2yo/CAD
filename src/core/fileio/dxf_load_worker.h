#pragma once

#include "dxf_reader.h"
#include <QThread>
#include <QObject>
#include <QString>
#include <memory>

namespace yjcad {

/// Background worker for loading DXF files without blocking UI.
/// Parses into a temporary Document to avoid data races with the renderer.
class DxfLoadWorker : public QObject {
    Q_OBJECT

public:
    explicit DxfLoadWorker(const QString& filePath);

    /// Transfer ownership of the parsed temporary document to caller.
    /// Only valid after finished(true,...) is emitted.
    std::unique_ptr<Document> takeDocument();

signals:
    void progress(int percent, const QString& message);
    void finished(bool success, const QString& message, int entityCount, int layerCount);
    void error(const QString& message);

public slots:
    void process();

private:
    QString m_filePath;
    std::unique_ptr<Document> m_tempDoc;
};

/// Convenience wrapper: spawns a QThread + DxfLoadWorker
class DxfLoadTask : public QObject {
    Q_OBJECT

public:
    DxfLoadTask(const QString& filePath, QObject* parent = nullptr);
    ~DxfLoadTask() override;

    void start();

    /// Transfer parsed document (call from main thread after finished signal)
    std::unique_ptr<Document> takeDocument();

signals:
    void progress(int percent, const QString& message);
    void finished(bool success, const QString& message, int entityCount, int layerCount);

private:
    QThread m_thread;
    DxfLoadWorker* m_worker;
};

} // namespace yjcad
