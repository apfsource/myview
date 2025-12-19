#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QTabWidget;

class QLocalServer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

public:
    void addImageTab(const QString &filePath);
    void openImageInNewTab(const QString &filePath); // Alias/Wrapper
    
    // Info Tabs
    void openInfoTab(const QString &title, const QString &content);

public slots:
    void showWelcomeTab();

private slots:
    void handleNewConnection();
    void closeTab(int index);
    void openFileDialog();
    
    // Status & Shortcuts
    void updateStatusBar(const QString &message);
    void closeCurrentTab();
    void nextTab();
    void prevTab();
    
    // Info Slots
    void showHelp();
    void showPrivacy();
    void showDisclaimer();
    void showTerms();

private:
    QTabWidget *m_tabWidget;
    QLocalServer *m_localServer;
    QString m_lastOpenPath;
};

#endif // MAINWINDOW_H
