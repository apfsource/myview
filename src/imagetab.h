#ifndef IMAGETAB_H
#define IMAGETAB_H

#include <QWidget>
#include <QPixmap>

class QPushButton;
class QLabel;
class QScrollArea;

class ImageTab : public QWidget
{
    Q_OBJECT
public:
    explicit ImageTab(const QString &filePath, QWidget *parent = nullptr);
    
    QString currentFilePath() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void statusChanged(const QString &message);

private slots:
    void showNextImage();
    void showPreviousImage();
    void zoomIn();
    void zoomOut();
    void resetZoom(); // Fit to screen
    void zoomActualSize(); // 100%

private:
    void updateHudPosition();
    void setupHud();

    QWidget *m_hudWidget; 
    // ... existing private members ...

private:
    void updateImageDisplay();
    void scanFolder();
    void loadImage(const QString &path);
    void updateCursor();

    QString m_currentFilePath;
    QPixmap m_originalPixmap;
    QLabel *m_imageLabel;
    QScrollArea *m_scrollArea;
    
    QStringList m_images;
    int m_currentIndex;
    bool m_loadSuccess;
    
    double m_zoomFactor;
    
    // Dragging state
    bool m_isDragging;
    QPoint m_lastMousePos;
};

#endif // IMAGETAB_H
