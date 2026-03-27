#include "dxf_load_worker.h"

namespace yjcad {

// ── DxfLoadWorker ──

DxfLoadWorker::DxfLoadWorker(Document& doc, const QString& filePath)
    : m_doc(doc), m_filePath(filePath) {}

void DxfLoadWorker::process() {
    emit progress(10, "파일 읽는 중...");

    DxfReader reader(m_doc);
    auto result = reader.load(m_filePath.toStdString());

    if (!result.success) {
        emit error(QString::fromStdString(result.errorMessage));
        emit finished(false, QString::fromStdString(result.errorMessage), 0, 0);
        return;
    }

    emit progress(100, "로드 완료");
    emit finished(true, "로드 완료",
                  static_cast<int>(result.entityCount),
                  static_cast<int>(result.layerCount));
}

// ── DxfLoadTask ──

DxfLoadTask::DxfLoadTask(Document& doc, const QString& filePath, QObject* parent)
    : QObject(parent) {
    m_worker = new DxfLoadWorker(doc, filePath);
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

} // namespace yjcad
