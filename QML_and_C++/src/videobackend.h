#pragma once
#include <QObject>
#include <QString>
#include <QTimer>
#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>

class VideoBackend : public QObject {
  Q_OBJECT
public:
  explicit VideoBackend(QObject *parent = nullptr);
  ~VideoBackend();

public slots:
  void loadModel(const QString &path);
  void loadVideo(const QString &path);
  void pauseVideo();   // Pause slot``
  void resumeVideo();  // Resume slot
  void restartVideo(); // Startup slot
  void stopVideo();

signals:
  void frameReady(const QImage &frame);
  void videoEnded();                      // Videoend signal
  void statusMessage(const QString &msg); // Statusbar signal

private slots:
  void processFrame();

private:
  QString m_videoPath;
  bool m_isModelLoaded;
  cv::dnn::Net m_net;
  cv::VideoCapture m_cap;
  QTimer m_timer;

  int m_frameCount;
  std::vector<cv::Rect> lastBoxes;
  std::vector<int> lastClassIds;
  std::vector<float> lastConfidences;
};
