#include "dxf_load_worker.h"
#include <stdexcept>

namespace yjcad {

// ── DxfLoadWorker ──

DxfLoadWorker::DxfLoadWorker(const QString& filePath)
    : m_filePath(filePath) {}

void DxfLoadWorker::process() {
    try {
        emit progress(10, "파일 읽는 중...");

        // Parse into a temporary document (thread-safe: no shared state)
        m_tempDoc = std::make_unique<Document>();
        DxfReader reader(*m_tempDoc);
        auto result = reader.load(m_filePath.toStdString());

        if (!result.success) {
            m_tempDoc.reset();
            emit error(QString::fromStdString(result.errorMessage));
            emit finished(false, QString::fromStdString(result.errorMessage), 0, 0);
            return;
        }

        emit progress(100, "로드 완료");
        emit finished(true, "로드 완료",
                      static_cast<int>(result.entityCount),
                      static_cast<int>(result.layerCount));
    } catch (const std::exception& e) {
        m_tempDoc.reset();
        QString msg = QString("DXF 로드 예외: %1").arg(e.what());
        emit error(msg);
        emit finished(false, msg, 0, 0);
    } catch (...) {
        m_tempDoc.reset();
        QString msg = "DXF 로드 중 알 수 없는 오류 발생";
        emit error(msg);
        emit finished(false, msg, 0, 0);
    }
}

std::unique_ptr<Document> DxfLoadWorker::takeDocument() {
    return std::move(m_tempDoc);
}

// ── DxfLoadTask ──

DxfLoadTask::DxfLoadTask(const QString& filePath, QObject* parent)
    : QObject(parent) {
    m_worker = new DxfLoadWorker(filePath);
    m_worker->moveToThread(&m_thread);

    connect(&m_thread, &QThread::started, m_worker, &DxfLoadWorker::process);
    connect(m_worker, &DxfLoadWorker::progress, this, &DxfLoadTask::progress);
    connect(m_worker, &DxfLoadWorker::finished, this, &DxfLoadTask::finished);
    connect(m_worker, &DxfLoadWorker::finished, &m_thread, &QThread::quit);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
}

DxfLoadTask::~DxfLoadTask() {
    if (m_thread.isRunning()) {
        m_thread.quit();
        m_thread.wait();
    }
}

void DxfLoadTask::start() {
    m_thread.start();
}

std::unique_ptr<Document> DxfLoadTask::takeDocument() {
    return m_worker ? m_worker->takeDocument() : nullptr;
}

} // namespace yjcad
