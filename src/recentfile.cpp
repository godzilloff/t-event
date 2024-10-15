#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSettings>
#include <QFileDialog>


void MainWindow::createActionsAndConnections(){
    QAction* recentFileAction = 0;
    for(auto i = 0; i < maxFileNr; ++i){
        recentFileAction = new QAction(this);
        recentFileAction->setVisible(false);
        QObject::connect(recentFileAction, &QAction::triggered,
                         this, &MainWindow::openRecent);
        recentFileActionList.append(recentFileAction);
    }
}

void MainWindow::createMenus(){
    fileMenu = ui->menubar->findChild<QMenu*>("menu");

    if (fileMenu != nullptr){
        recentFilesMenu = new QMenu(tr("Открыть последние"),this);
        fileMenu->insertMenu(ui->act_save_as, recentFilesMenu);
        for(auto i = 0; i < maxFileNr; ++i)
            recentFilesMenu->addAction(recentFileActionList.at(i));
        updateRecentActionList();
    }
}

void MainWindow::openRecent(){
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        open_JSON(action->data().toString());
}

void MainWindow::adjustForCurrentFile(const QString &filePath){
    currentFilePath = filePath;
    setWindowFilePath(currentFilePath);

    QSettings settings("recentfile.ini", QSettings::IniFormat);
    QStringList recentFilePaths =
            settings.value("recentFiles").toStringList();
    recentFilePaths.removeAll(filePath);
    recentFilePaths.prepend(filePath);
    while (recentFilePaths.size() > maxFileNr)
        recentFilePaths.removeLast();
    settings.setValue("recentFiles", recentFilePaths);

    // see note
    updateRecentActionList();
    updateWindowTitle();
}

void MainWindow::updateWindowTitle(){
    QString title = SEvent.getNameEvent() + " ["+currentFilePath+"]" + " - T-Event 0.5.1";
    this->setWindowTitle(title);
}

void MainWindow::updateRecentActionList(){
    QSettings settings("recentfile.ini", QSettings::IniFormat);
    QStringList recentFilePaths =
            settings.value("recentFiles").toStringList();

    auto itEnd = 0;
    if(recentFilePaths.size() <= maxFileNr)
        itEnd = recentFilePaths.size();
    else
        itEnd = maxFileNr;

    for (auto i = 0; i < itEnd; ++i) {
        QString strippedName = QFileInfo(recentFilePaths.at(i)).fileName();
        recentFileActionList.at(i)->setText(strippedName);
        recentFileActionList.at(i)->setData(recentFilePaths.at(i));
        recentFileActionList.at(i)->setVisible(true);
    }

    for (auto i = itEnd; i < maxFileNr; ++i)
        recentFileActionList.at(i)->setVisible(false);
}
