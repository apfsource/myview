#include "mainwindow.h"
#include "imagetab.h"
#include <QTabWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDataStream>
#include <QApplication>
#include <QPushButton>
#include <QFileDialog>
#include <QIcon>

#include <QToolButton>
#include <QStatusBar>
#include <QShortcut>
#include <QKeySequence>
#include <QTextBrowser>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(new QTabWidget(this))
    , m_localServer(new QLocalServer(this))
    , m_lastOpenPath(QDir::homePath())
{
    // 2. Window behavior: Set window title and reasonable default size
    setWindowTitle("MyView");
    
    // Explicitly set the window icon to the new logo
    setWindowIcon(QIcon(":/logo.png"));
    
    resize(800, 600);
    setAcceptDrops(true); // Enable Drag & Drop
    
    // Status Bar
    statusBar()->showMessage("Ready");
    
    // Status Bar Helper: Create clickable label/button style
    auto addStatusBtn = [&](const QString &text, auto slot) {
        QPushButton *btn = new QPushButton(text, this);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("QPushButton { border: none; padding: 2px 10px; color: #a0a0a0; font-size: 11px; } QPushButton:hover { color: #ffffff; }");
        connect(btn, &QPushButton::clicked, this, slot);
        statusBar()->addPermanentWidget(btn);
    };
    
    addStatusBtn("Terms", &MainWindow::showTerms);
    addStatusBtn("Disclaimer", &MainWindow::showDisclaimer);
    addStatusBtn("Privacy", &MainWindow::showPrivacy);
    addStatusBtn("Help", &MainWindow::showHelp);

    // 3. Tab system: Use QTabWidget as central widget
    setCentralWidget(m_tabWidget);
    m_tabWidget->setTabsClosable(true);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    
    // Status updates from tabs
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index >= 0) {
            ImageTab *tab = qobject_cast<ImageTab*>(m_tabWidget->widget(index));
            if (tab) {
                // Focus the tab to ensure key events work immediately
                tab->setFocus();
                // We rely on the tab emiting status later or we should store it?
                // For now, reset status or wait for interaction. 
                // Better: ImageTab *could* report on re-show, which we implemented in ImageTab::showEvent.
            } else {
                statusBar()->showMessage("Ready");
            }
        }
    });
    
    // Shortcuts
    new QShortcut(QKeySequence::Close, this, SLOT(closeCurrentTab())); // Ctrl+W
    new QShortcut(QKeySequence::NextChild, this, SLOT(nextTab())); // Ctrl+Tab
    new QShortcut(QKeySequence::PreviousChild, this, SLOT(prevTab())); // Ctrl+Shift+Tab
    new QShortcut(QKeySequence::New, this, SLOT(showWelcomeTab())); // Ctrl+N
    // Note: Ctrl+Tab might be consumed by QTabWidget by default, but explicit shortcut ensures it.
    
    // Corner Widget for "New Tab" button
    QToolButton *newTabBtn = new QToolButton(this);
    newTabBtn->setText("+");
    newTabBtn->setToolTip("Open New Tab");
    newTabBtn->setObjectName("newTabButton"); // For QSS targeting
    newTabBtn->setAutoRaise(true); 
    connect(newTabBtn, &QToolButton::clicked, this, &MainWindow::showWelcomeTab);
    
    m_tabWidget->setCornerWidget(newTabBtn, Qt::TopRightCorner);
    
    // Default welcome tab logic moved to showWelcomeTab()
    showWelcomeTab();
    
    // IPC Server Setup
    // Cleanup any stale socket file
    QLocalServer::removeServer("myview_server");
    
    if (m_localServer->listen("myview_server")) {
        connect(m_localServer, &QLocalServer::newConnection, this, &MainWindow::handleNewConnection);
    } 
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeTab(int index)
{
    QWidget *widget = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    if (widget) {
        widget->deleteLater();
    }
    
    // If no tabs left, show welcome to keep app usable
    if (m_tabWidget->count() == 0) {
        showWelcomeTab();
    }
}

void MainWindow::openFileDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", m_lastOpenPath, "Images (*.png *.jpg *.jpeg *.bmp *.webp)");
    if (!fileName.isEmpty()) {
        openImageInNewTab(fileName);
    }
}

void MainWindow::openImageInNewTab(const QString &filePath)
{
    addImageTab(filePath);
}

// ... existing includes ...
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>

// ... constructor unchanged ...

// D&D Implementation
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            openImageInNewTab(url.toLocalFile());
        }
    }
    event->acceptProposedAction();
}

void MainWindow::addImageTab(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    QString absolutePath = fileInfo.canonicalFilePath(); // Robust path check
    if (absolutePath.isEmpty()) absolutePath = fileInfo.absoluteFilePath(); // Fallback
    
    // Update last open path to this file's directory
    m_lastOpenPath = fileInfo.absolutePath();
    
    // Duplicate Check using canonical path
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        ImageTab *existingTab = qobject_cast<ImageTab*>(m_tabWidget->widget(i));
        // We need to compare specific property or ask tab. 
        // ImageTab::currentFilePath() returns absolute. But maybe we need canonical comparison.
        if (existingTab) {
             QFileInfo existingInfo(existingTab->currentFilePath());
             if (existingInfo.canonicalFilePath() == absolutePath) {
                 m_tabWidget->setCurrentIndex(i);
                 return;
             }
        }
    }
    
    // If Duplicate Logic found nothing, check if we should REPLACE "Welcome" tab?
    // User requested "application jitani baar khola ja raha hai welcome pae utani baar new ban raha hai"
    // -> "Welcome" tabs should not accumulate.
    // If we are opening an image, we probably don't need *any* welcome tabs anymore? 
    // Or just don't open new ones. 
    // Strategy: If current tab is "Welcome", remove it? No, maybe user wants to keep it.
    // But let's ensuring we don't open image IF it's already there (done above).
    
    // Create new.
    ImageTab *tab = new ImageTab(absolutePath, this);
    connect(tab, &ImageTab::statusChanged, this, &MainWindow::updateStatusBar);
    
    m_tabWidget->addTab(tab, fileInfo.fileName());
    m_tabWidget->setCurrentWidget(tab);
}

void MainWindow::handleNewConnection()
{
    QLocalSocket *socket = m_localServer->nextPendingConnection();
    if (!socket) return;
    
    connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
        QDataStream in(socket);
        QStringList args;
        in >> args;
        
        for (const QString &arg : args) {
             openImageInNewTab(arg);
        }
        
        // Bring window to front
        this->show();
        this->raise();
        this->activateWindow();
        
        socket->deleteLater();
    });
}


void MainWindow::showWelcomeTab()
{
    QWidget *placeholderTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(placeholderTab);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter); // vertical center entire content
    
    // Icon
    QLabel *iconLabel = new QLabel(placeholderTab);
    iconLabel->setPixmap(QIcon(":/logo.png").pixmap(128, 128));
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);
    
    // Welcome Text
    QLabel *label = new QLabel("Welcome to MyView", placeholderTab);
    label->setAlignment(Qt::AlignCenter);
    
    // Bold, larger font
    QFont font = label->font();
    font.setPointSize(16);
    font.setBold(true);
    label->setFont(font);
    
    layout->addWidget(label);
    
    // Subtext
    QLabel *subLabel = new QLabel("Open an image to get started", placeholderTab);
    subLabel->setAlignment(Qt::AlignCenter);
    subLabel->setStyleSheet("color: #a0a0a0;");
    layout->addWidget(subLabel);
    
    // Button Container
    QHBoxLayout *btnLayout = new QHBoxLayout();
    
    QPushButton *openBtn = new QPushButton("Open Image", placeholderTab);
    openBtn->setCursor(Qt::PointingHandCursor);
    // Explicit sizing for a nice big button
    openBtn->setMinimumSize(150, 45);
    
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::openFileDialog);
    
    btnLayout->addStretch();
    btnLayout->addWidget(openBtn);
    btnLayout->addStretch();
    
    layout->addLayout(btnLayout);

    // Duplicate Check for Welcome Tab
    for (int i = 0; i < m_tabWidget->count(); ++i) {
         if (m_tabWidget->tabText(i) == "Welcome") {
             m_tabWidget->setCurrentIndex(i);
             // Should we update the existing? It's static, so just activating is fine.
             // Delete the temp pointer we created?
             delete placeholderTab; 
             return;
         }
    }

    m_tabWidget->addTab(placeholderTab, "Welcome");
    m_tabWidget->setCurrentWidget(placeholderTab);
}

void MainWindow::updateStatusBar(const QString &message)
{
    statusBar()->showMessage(message);
}

void MainWindow::closeCurrentTab()
{
    int index = m_tabWidget->currentIndex();
    if (index >= 0) {
        closeTab(index);
    }
}

void MainWindow::nextTab()
{
    int next = m_tabWidget->currentIndex() + 1;
    if (next < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(next);
    } else {
        m_tabWidget->setCurrentIndex(0); // Cycle
    }
}

void MainWindow::prevTab()
{
    int prev = m_tabWidget->currentIndex() - 1;
    if (prev >= 0) {
        m_tabWidget->setCurrentIndex(prev);
    } else {
        m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1); // Cycle
    }
}

void MainWindow::openInfoTab(const QString &title, const QString &content)
{
    // Check if tab already exists
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (m_tabWidget->tabText(i) == title) {
            m_tabWidget->setCurrentIndex(i);
            return;
        }
    }
    
    // Create new tab
    QTextBrowser *browser = new QTextBrowser();
    browser->setOpenExternalLinks(true);
    browser->setHtml(content);
    // Style text browser darkly
    browser->setStyleSheet("QTextBrowser { background-color: #2b2b2b; color: #e0e0e0; border: none; font-size: 14px; padding: 20px; }");
    
    m_tabWidget->addTab(browser, title);
    m_tabWidget->setCurrentWidget(browser);
}

void MainWindow::showHelp()
{
    QString content = 
        "<h1>MyView Help</h1>"
        "<p>Welcome to MyView, a modern and fast MyView.</p>"
        "<h2>Features</h2>"
        "<ul>"
        "<li><b>Smart Zoom:</b> Double-click an image to toggle between 'Fit to Screen' and 'Actual Size'.</li>"
        "<li><b>HUD Controls:</b> Hover over the bottom of an image to see navigation and zoom controls.</li>"
        "<li><b>Drag & Drop:</b> Drag images directly into the window to open them.</li>"
        "<li><b>Transparency:</b> Transparent images are shown with a checkerboard background.</li>"
        "</ul>"
        "<h2>Shortcuts</h2>"
        "<ul>"
        "<li><b>Arrow Keys:</b> Navigate images</li>"
        "<li><b>Ctrl + N:</b> New Tab (Welcome)</li>"
        "<li><b>Ctrl + W:</b> Close Tab</li>"
        "<li><b>Ctrl + Scroll:</b> Zoom In/Out</li>"
        "</ul>";
    openInfoTab("Help", content);
}

void MainWindow::showPrivacy()
{
    QString content = 
        "<h1>Privacy Policy</h1>"
        "<p>MyView is a local offline application.</p>"
        "<ul>"
        "<li>We do not collect any personal data.</li>"
        "<li>No images are uploaded to any server.</li>"
        "<li>All processing happens locally on your machine.</li>"
        "</ul>";
    openInfoTab("Privacy", content);
}

void MainWindow::showDisclaimer()
{
    QString content = 
        "<h1>Disclaimer</h1>"
        "<p>This software is provided 'as is', without warranty of any kind.</p>"
        "<p>The authors are not liable for any damages arising from the use of this software.</p>";
    openInfoTab("Disclaimer", content);
}

void MainWindow::showTerms()
{
    QString content = 
        "<h1>Terms and Conditions</h1>"
        "<p>By using MyView, you agree to the following terms:</p>"
        "<ul>"
        "<li>You may use this software for personal or commercial purposes.</li>"
        "<li>You may not reverse engineer or redistribute this software without permission.</li>"
        "</ul>";
    openInfoTab("Terms", content);
}
