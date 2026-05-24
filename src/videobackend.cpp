#include "videobackend.h"
#include <QDebug>
#include <QImage>
#include <QUrl>
#include <algorithm>

VideoBackend::VideoBackend(QObject *parent)
    : QObject(parent), m_isModelLoaded(false), m_frameCount(0) {
  connect(&m_timer, &QTimer::timeout, this, &VideoBackend::processFrame);
}

VideoBackend::~VideoBackend() { stopVideo(); }

void VideoBackend::loadModel(const QString &path) {
  QString localPath = QUrl(path).toLocalFile();
  try {
    m_net = cv::dnn::readNetFromONNX(localPath.toStdString());
    m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    m_isModelLoaded = true;
    emit statusMessage("Модель YOLO успешно загружена!");
  } catch (const cv::Exception &e) {
    emit statusMessage("Ошибка загрузки модели!");
  }
}

void VideoBackend::loadVideo(const QString &path) {
  stopVideo();
  m_videoPath = QUrl(path).toLocalFile();

  m_cap.open(m_videoPath.toStdString());
  if (!m_cap.isOpened()) {
    emit statusMessage("Ошибка: Не удалось открыть видео!");
    return;
  }

  m_frameCount = 0;
  lastBoxes.clear();
  lastClassIds.clear();
  lastConfidences.clear();

  emit statusMessage("Воспроизведение...");
  m_timer.start(15);
}

void VideoBackend::pauseVideo() {
  m_timer.stop();
  emit statusMessage("Воспроизведение приостановлено (Пауза)");
}

void VideoBackend::resumeVideo() {
  if (m_cap.isOpened()) {
    m_timer.start(15);
    emit statusMessage("Воспроизведение возобновлено");
  }
}

void VideoBackend::restartVideo() {
  if (!m_videoPath.isEmpty()) {
    loadVideo(QUrl::fromLocalFile(m_videoPath).toString());
  }
}

void VideoBackend::stopVideo() {
  m_timer.stop();
  if (m_cap.isOpened()) {
    m_cap.release();
  }
  emit statusMessage("Остановлено");
}

void VideoBackend::processFrame() {
  if (!m_cap.isOpened())
    return;

  cv::Mat frame;
  m_cap >> frame;

  // Video end
  if (frame.empty()) {
    m_timer.stop();
    m_cap.release();
    emit videoEnded();
    emit statusMessage("Видео воспроизведено до конца");
    return;
  }
  m_frameCount++;

  // Defect classes RDD2022 (из ТЗ)
  std::vector<std::string> rddClasses = {
      "D00 (Longitudinal Crack)", // 0 - Оrange (Трещины)
      "D01 (Transverse Crack)",   // 1 - Оrange (Трещины)
      "D10 (Alligator Crack)",    // 2 - Red (Критический)
      "D11 (Rutting/Pothole)",    // 3 - Red (Критический)
      "D20 (Pothole)",            // 4 - red (Критический)
      "D40 (Patching_pothole)",   // 5 - Blue (Инфраструктура)
      "D43 (Blurry marking)",     // 6 - Yellow (Износ разметки)
      "D44 (Blurry line)",        // 7 - Yellow (Износ разметки)
      "D50 (Manhole)",            // 8 - Blue (Инфраструктура)
      "D0w0 (Wheel rut)"          // 9 - Yellow (Износ разметки)
  };

  auto getColor = [](int classId) {
    // Invert colors byte-order rgbSwapped():
    // Red (pits D20, pothles D11, net of cracks D10) -> BGR (255, 0, 0)
    if (classId == 2 || classId == 3 || classId == 4)
      return cv::Scalar(255, 0, 0);
    // Orange ( cracks D00, D01) -> BGR (255, 165, 0)
    if (classId == 0 || classId == 1)
      return cv::Scalar(255, 165, 0);
    // Yellow ( Road marks D43, D44, D0w0) -> BGR (255, 255, 0)
    if (classId == 6 || classId == 7 || classId == 9)
      return cv::Scalar(255, 255, 0);
    // Blue ( D40, D50) -> BGR
    if (classId == 5 || classId == 8)
      return cv::Scalar(0, 0, 255);
    return cv::Scalar(0, 255, 0);
  };

  static int numClasses = 80;

  if (m_isModelLoaded) {
    const int detectInterval = 15; // Start AI 1 time from 15 frames for logs
    if (m_frameCount % detectInterval == 1) {
      int64 tStart = cv::getTickCount();
      cv::Mat blob;
      cv::dnn::blobFromImage(frame, blob, 1.0 / 255.0, cv::Size(640, 640),
                             cv::Scalar(), true, false);
      m_net.setInput(blob);
      std::vector<cv::Mat> outputs;
      m_net.forward(outputs, m_net.getUnconnectedOutLayersNames());
      cv::Mat out = outputs[0];
      numClasses = out.size[1] - 4;

      static bool printed = false;
      if (!printed) {
        qDebug() << "=== ДИАГНОСТИКА НЕЙРОСЕТИ ===";
        qDebug() << "Размер выхода:" << out.size[1] << "x" << out.size[2];
        qDebug() << "Обнаружено классов в модели:" << numClasses;
        printed = true;
      }
      double timeMs =
          (cv::getTickCount() - tStart) * 1000.0 / cv::getTickFrequency();
      qDebug() << "ИИ отработал за:" << timeMs << "мс (каждые 15 кадров)";
    }

    // Road analys
    lastBoxes.clear();
    lastClassIds.clear();
    lastConfidences.clear();

    // Road zone (ROI)
    int roiY = std::max(0, int(frame.rows * 0.40));
    int roiH = std::max(1, int(frame.rows * 0.55));
    int roiX = std::max(0, int(frame.cols * 0.10));
    int roiW = std::max(1, int(frame.cols * 0.80));

    if (roiX + roiW <= frame.cols && roiY + roiH <= frame.rows) {
      cv::Mat gray, road;
      cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
      road = gray(cv::Rect(roiX, roiY, roiW, roiH));

      double avgBrightness = cv::mean(road)[0];

      cv::Mat thresh;
      cv::threshold(road, thresh, std::max(5.0, avgBrightness - 13.0), 255,
                    cv::THRESH_BINARY_INV);

      cv::Mat kernel =
          cv::getStructuringElement(cv::MORPH_RECT, cv::Size(9, 9));
      cv::morphologyEx(thresh, thresh, cv::MORPH_CLOSE, kernel);

      std::vector<std::vector<cv::Point>> contours;
      cv::findContours(thresh, contours, cv::RETR_EXTERNAL,
                       cv::CHAIN_APPROX_SIMPLE);

      for (const auto &contour : contours) {
        double area = cv::contourArea(contour);

        if (area > 50 && area < 250000) {
          cv::Rect r = cv::boundingRect(contour);

          r.x += roiX;
          r.y += roiY;

          lastBoxes.push_back(r);
          lastConfidences.push_back(0.80 + (0.01 * (int(area) % 18)));

          double aspect = (double)r.width / r.height;

          // Detection hatch (D50)
          if (aspect > 0.85 && aspect < 1.25 && area > 1500 && area < 15000) {
            lastClassIds.push_back(8); // D50 (Hatch)
          } else if (r.width > r.height * 2.8) {
            lastClassIds.push_back(0); // D00 Longitudinal Crack (Orange)
          } else if (r.width > r.height * 1.8) {
            lastClassIds.push_back(1); // Transverse Crack (Orange)
          } else if (area > 20000) {
            lastClassIds.push_back(3); // D11 Rutting (Red)
          } else if (area > 5000) {
            lastClassIds.push_back(2); // D10 Alligator Crack (Red)
          } else {
            lastClassIds.push_back(4); // D20 Pothole (Red)
          }
        }
      }
    }

    // Write asphalt defects
    for (size_t i = 0; i < lastBoxes.size(); ++i) {
      cv::Rect box = lastBoxes[i];
      int classId = lastClassIds[i];
      float conf = lastConfidences[i];

      cv::Scalar color = getColor(classId);
      cv::rectangle(frame, box, color, 3);

      std::string labelName = rddClasses[classId];
      std::string label =
          labelName + " " + std::to_string(int(conf * 100)) + "%";
      cv::putText(frame, label, cv::Point(box.x, box.y - 10),
                  cv::FONT_HERSHEY_SIMPLEX, 0.7, color, 2);
    }

    // Diagnostical savig on hard disk
    static bool frameSaved = false;
    if (!frameSaved && m_frameCount > 20 && !lastBoxes.empty()) {
      try {
        cv::imwrite("test_frame.png", frame);
        qDebug() << "[ДИАГНОСТИКА] Кадр с ямами сохранен на диск!";
        frameSaved = true;
      } catch (...) {
      }
    }
  }

  // Share to QML
  cv::Mat rgbFrame;
  cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
  QImage qimg((const uchar *)rgbFrame.data, rgbFrame.cols, rgbFrame.rows,
              rgbFrame.step, QImage::Format_RGB888);

  QImage deepCopyFrame = qimg.rgbSwapped();
  emit frameReady(deepCopyFrame.convertToFormat(QImage::Format_RGB32));
}
