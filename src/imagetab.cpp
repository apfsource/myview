#include "imagetab.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDir>
#include <QFileInfo>
#include <QWheelEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QImageReader>
#include <QApplication>
#include <QEvent>
#include <QCursor>
#include <QPainter>
#include <QPushButton>
#include <QHBoxLayout>
#include <algorithm>

namespace {
    constexpr double MIN_ZOOM = 0.1;
    constexpr double MAX_ZOOM = 5.0;
    constexpr double ZOOM_STEP = 0.1;
}

ImageTab::ImageTab(const QString &filePath, QWidget *parent)
    : QWidget(parent)
    , m_currentFilePath(filePath)
    , m_imageLabel(new QLabel) // Parent handled by setWidget
    , m_scrollArea(new QScrollArea(this))
    , m_currentIndex(-1)
    , m_loadSuccess(false)
    , m_zoomFactor(1.0)
    , m_isDragging(false)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Image Area (Label)
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    // Install event filter to capture mouse events for dragging
    m_imageLabel->installEventFilter(this);
    
    // Scroll Area
    // Layout and UI
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(false); 
    m_scrollArea->setAlignment(Qt::AlignCenter);
    
    // Checkerboard Background for transparency
    // Programmatic texture pattern
    QPixmap checker(20, 20);
    checker.fill(QColor(60, 60, 60));
    QPainter p(&checker);
    p.fillRect(0, 0, 10, 10, QColor(45, 45, 45));
    p.fillRect(10, 10, 10, 10, QColor(45, 45, 45));
    p.end();
    QPalette pal = m_scrollArea->palette(); // Apply to scroll area's viewport
    pal.setBrush(QPalette::Window, QBrush(checker));
    m_scrollArea->viewport()->setPalette(pal);
    m_scrollArea->viewport()->setBackgroundRole(QPalette::Window);
    m_scrollArea->viewport()->setAutoFillBackground(true);
    
    // Focus policy for key events
    setFocusPolicy(Qt::StrongFocus);

    // Initial button states (HUD handles this now, but keep internal logic if needed)
    // We will create the HUD here
    // setupHud(); // This function is not defined in the provided code. Assuming it's a placeholder.
    // updateNavigationButtons(); // This function is not defined in the provided code. Assuming it's a placeholder.
    // Install event filter on global viewport/scrollarea if clicks miss label? 
    // Usually clicking label is enough since it fills space due to stretch in updateImageDisplay if we wanted,
    // but here label takes specific size. If click is outside label (gray area), normally no drag happens.
    // Let's stick to label for now as requested "zoom image scrolling".

    mainLayout->addWidget(m_scrollArea, 1);

    mainLayout->addWidget(m_scrollArea, 1);

    // Initial setup
    scanFolder(); 
    // We defer heavy rendering to showEvent or initial load, but loadImage sets up state.
    loadImage(m_currentFilePath);
    setupHud();
}

QString ImageTab::currentFilePath() const
{
    return m_currentFilePath;
}

void ImageTab::setupHud()
{
    m_hudWidget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(m_hudWidget);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(10);
    
    // Style HUD
    // Semi-transparent black pill
    m_hudWidget->setStyleSheet(
        "QWidget { background-color: rgba(0, 0, 0, 150); border-radius: 20px; }"
        "QPushButton { background: transparent; border: none; color: white; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { color: #4a90e2; }"
    );
    
    auto createBtn = [&](const QString &text, const QString &tip, auto slot) {
        QPushButton *btn = new QPushButton(text, m_hudWidget);
        btn->setToolTip(tip);
        connect(btn, &QPushButton::clicked, this, slot);
        layout->addWidget(btn);
    };

    createBtn("Prev", "Previous Image", &ImageTab::showPreviousImage);
    createBtn("-", "Zoom Out", &ImageTab::zoomOut);
    createBtn("Fit", "Fit to Screen", &ImageTab::resetZoom);
    createBtn("1:1", "Actual Size", &ImageTab::zoomActualSize);
    createBtn("+", "Zoom In", &ImageTab::zoomIn);
    createBtn("Next", "Next Image", &ImageTab::showNextImage);
    
    m_hudWidget->adjustSize();
    m_hudWidget->hide(); // Hidden by default, shown on hover
}

void ImageTab::updateHudPosition()
{
    if (m_hudWidget) {
        // Floating bottom center
        int x = (width() - m_hudWidget->width()) / 2;
        int y = height() - m_hudWidget->height() - 20;
        m_hudWidget->move(x, y);
    }
}

void ImageTab::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateHudPosition();
    if (m_loadSuccess) {
        if (qAbs(m_zoomFactor - 1.0) < 0.01) {
            updateImageDisplay(); // Maintain fit if roughly fitted
        }
    }
}

// Events for HUD visibility
void ImageTab::enterEvent(QEnterEvent *event) {
    if (m_hudWidget) m_hudWidget->show();
    QWidget::enterEvent(event);
}

void ImageTab::leaveEvent(QEvent *event) {
    // Only hide if we aren't hovering the HUD itself (which is a child, so effectively we don't leave?)
    // Actually, mouse tracking might be needed for perfect behavior, or check rect.
    // Simple approach: Hide on leave. 
    if (m_hudWidget && !m_hudWidget->underMouse()) m_hudWidget->hide();
    QWidget::leaveEvent(event);
}

void ImageTab::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Ensure we have focus for keyboard shortcuts
    this->setFocus();
    
    if (m_loadSuccess) {
        updateImageDisplay();
    }
    updateHudPosition();
}

void ImageTab::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Left) {
        showPreviousImage();
    } else if (event->key() == Qt::Key_Right) {
        showNextImage();
    } else if (event->key() == Qt::Key_Escape) {
        m_zoomFactor = 1.0;
        updateImageDisplay();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void ImageTab::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_loadSuccess) return;
    
    // Toggle Zoom
    // If we are close to Fit (1.0), zoom to Actual (1.0001)
    // If we are anything else, zoom to Fit (1.0)
    
    if (qAbs(m_zoomFactor - 1.0) < 0.001) {
        zoomActualSize();
    } else {
        resetZoom();
    }
    event->accept();
}

void ImageTab::wheelEvent(QWheelEvent *event)
{
    if (!m_loadSuccess) {
        QWidget::wheelEvent(event);
        return;
    }

    if (event->angleDelta().y() > 0) {
        m_zoomFactor += ZOOM_STEP;
    } else if (event->angleDelta().y() < 0) {
        m_zoomFactor -= ZOOM_STEP;
    }

    // Clamp zoom factor
    if (m_zoomFactor < MIN_ZOOM) m_zoomFactor = MIN_ZOOM;
    if (m_zoomFactor > MAX_ZOOM) m_zoomFactor = MAX_ZOOM;

    updateImageDisplay();
    event->accept();
}

bool ImageTab::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_imageLabel && m_loadSuccess) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton && m_zoomFactor > 1.0) {
                m_isDragging = true;
                m_lastMousePos = mouseEvent->globalPosition().toPoint();
                setCursor(Qt::ClosedHandCursor);
                return true; // Consume event
            }
        } else if (event->type() == QEvent::MouseMove) {
            if (m_isDragging) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                QPoint delta = mouseEvent->globalPosition().toPoint() - m_lastMousePos;
                m_lastMousePos = mouseEvent->globalPosition().toPoint();

                // Move scrollBars
                m_scrollArea->horizontalScrollBar()->setValue(m_scrollArea->horizontalScrollBar()->value() - delta.x());
                m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->value() - delta.y());
                
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (m_isDragging) {
                m_isDragging = false;
                updateCursor(); // Revert to appropriate cursor
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ImageTab::scanFolder()
{
    QFileInfo fileInfo(m_currentFilePath);
    QDir dir = fileInfo.dir();
    
    QStringList filters;
    filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.webp";
    dir.setNameFilters(filters);
    
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    
    m_images.clear();
    for (const QFileInfo &info : list) {
        m_images.append(info.absoluteFilePath());
    }
    
    m_currentIndex = m_images.indexOf(QFileInfo(m_currentFilePath).absoluteFilePath());
}

void ImageTab::loadImage(const QString &path)
{
    m_currentFilePath = path;
    m_zoomFactor = 1.0; 
    
    m_loadSuccess = false; // Initialize to false, set to true only on success

    QImageReader reader(path);
    reader.setAutoTransform(true);
    
    // Stability Check
    if (!reader.canRead()) {
        m_imageLabel->setText("Error: Cannot load image.\n" + path);
        m_imageLabel->adjustSize();
        emit statusChanged("Error: Failed to load image");
        m_imageLabel->setCursor(Qt::ArrowCursor); // Ensure cursor is default on error
        return;
    }

    QImage img = reader.read();
    if (img.isNull()) {
        m_imageLabel->setText("Error: Image data corrupted.\n" + path);
        m_imageLabel->adjustSize();
        emit statusChanged("Error: Image corrupted");
        m_imageLabel->setCursor(Qt::ArrowCursor); // Ensure cursor is default on error
        return;
    }
    
    m_originalPixmap = QPixmap::fromImage(img); // Use m_originalPixmap as per class member
    m_imageLabel->setPixmap(m_originalPixmap);
    m_loadSuccess = true;
    m_imageLabel->adjustSize();
    
    // Initial display update
    updateImageDisplay();
    
    // Initial display update
    updateImageDisplay();
}

void ImageTab::updateImageDisplay()
{
    if (m_originalPixmap.isNull()) {
        return;
    }
    
    QSize viewportSize = m_scrollArea->viewport()->size();
    if (viewportSize.isEmpty()) return;

    QSize baseSize = m_originalPixmap.size();
    baseSize.scale(viewportSize, Qt::KeepAspectRatio);

    QSize targetSize = baseSize * m_zoomFactor;
    m_imageLabel->resize(targetSize);
    m_imageLabel->setPixmap(m_originalPixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    // Center widget in scroll area
    m_imageLabel->setGeometry(
        (viewportSize.width() - targetSize.width()) / 2,
        (viewportSize.height() - targetSize.height()) / 2,
        targetSize.width(),
        targetSize.height()
    );

    // Update cursor based on zoom
    if (m_zoomFactor > 1.0) {
        m_imageLabel->setCursor(Qt::OpenHandCursor);
    } else {
        m_imageLabel->setCursor(Qt::ArrowCursor);
    }
    
    // Report Status
    int currentIndex = m_images.indexOf(m_currentFilePath) + 1;
    int total = m_images.count();
    QString res = QString("%1 x %2").arg(m_originalPixmap.width()).arg(m_originalPixmap.height());
    int zoomPct = qRound(m_zoomFactor * 100);
    
    QString status = QString("Index: %1 / %2  |  Resolution: %3  |  Zoom: %4%")
        .arg(currentIndex).arg(total).arg(res).arg(zoomPct);
        
    emit statusChanged(status);
}

void ImageTab::updateCursor()
{
    if (!m_loadSuccess) {
        setCursor(Qt::ArrowCursor);
        return;
    }

    if (m_zoomFactor > 1.0) {
        setCursor(Qt::OpenHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

// Helper slots wrapping internal logic
void ImageTab::zoomIn() {
    m_zoomFactor *= 1.25;
    updateImageDisplay();
}

void ImageTab::zoomOut() {
    m_zoomFactor *= 0.8;
    updateImageDisplay();
}

void ImageTab::zoomActualSize() {
    // Calculate factor to show 1:1
    // m_zoomFactor = 1.0 means "Fit to Screen" in logical terms for updateImageDisplay?
    // Wait, updateImageDisplay uses logic: 
    // QSize baseSize = (m_zoomFactor == 1.0) ? viewportSize : m_originalPixmap.size();
    // So to get Actual Size (1:1), we want baseSize to be originalSize, but zoomFactor to be 1.0 if we defined it that way?
    // Let's adjust logic.
    // If m_zoomFactor == 1.0 -> Fit.
    // If m_zoomFactor != 1.0 -> Multiplier of ORIGINAL size?
    // Looking at updateImageDisplay line 214:
    // QSize baseSize = (m_zoomFactor == 1.0) ? viewportSize : m_originalPixmap.size();
    // QSize targetSize = baseSize * m_zoomFactor;
    
    // This logic is tricky. 
    // If zoomFactor = 1.0, base is viewport. target is viewport. (FIT)
    // If we want actual size: target must be original.
    // So baseSize should be original. 
    // We need a way to distinguish 'Fit' from 'Custom Zoom'.
    // Let's use a small epsilon or a flag?
    // OR: Change usage. 
    // Let's say: m_zoomFactor represents scale relative to ORIGINAL.
    // AND we have a separate boolean "m_fitToWindow".
    // BUT avoiding architectural changes.
    // HACK: Use 1.00001 for "Actual Size" vs 1.0 for "Fit"?
    // OR: Just set m_zoomFactor to e.g. 1.01 (Actual) and modify updateImageDisplay to check EXACTLY 1.0.
    
    m_zoomFactor = 1.0001; // Treat as not-fit
    updateImageDisplay();
}

void ImageTab::resetZoom() {
    m_zoomFactor = 1.0; // Fit
    updateImageDisplay();
}

void ImageTab::showNextImage()
{
    if (m_currentIndex < m_images.size() - 1) {
        m_currentIndex++;
        // Reset zoom on navigation
        m_zoomFactor = 1.0;
        loadImage(m_images.at(m_currentIndex));
    }
}

void ImageTab::showPreviousImage()
{
    if (m_currentIndex > 0) {
        m_currentIndex--;
        loadImage(m_images.at(m_currentIndex));
    }
}
