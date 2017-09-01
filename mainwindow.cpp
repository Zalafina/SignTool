/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include <QtWebKitWidgets>
#include "mainwindow.h"
#include "signtool.h"

//! [0]
MainWindow::MainWindow() :
    m_SysTrayIcon(NULL)
{
    createActions();
    createMenus();
    centralWidget = new SignTool(this);
    setCentralWidget(centralWidget);

    //connect(centralWidget->webView, SIGNAL(loadFinished(bool)), this, SLOT(updateTextEdit()));
    setStartupText();

    m_SysTrayIcon = new QSystemTrayIcon(this);
    m_SysTrayIcon->setIcon(QIcon(":/AppIcon.ico"));
    m_SysTrayIcon->setToolTip("Kamigami签到工具");
    m_SysTrayIcon->show();

    connect(m_SysTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(SystrayIconActivated(QSystemTrayIcon::ActivationReason)));

    centralWidget->loadSettings();

    centralWidget->setNetworkManager(centralWidget->webView->page()->networkAccessManager());

    centralWidget->webView->setUrl(SignTool::getHomeUrl());

    //QObject::connect(QApplication::instance(), SIGNAL(showUp()), this, SLOT(raiseWindow()));
    //QObject::connect(QApplication::instance(), SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(raiseWindow(Qt::ApplicationState)));
}
//! [0]


void MainWindow::changeEvent(QEvent *event)
{
    if(event->type()==QEvent::WindowStateChange)
    {
        QTimer::singleShot(0, this, SLOT(WindowStateChangedProc()));
    }
    QMainWindow::changeEvent(event);
}

//! [1]
void MainWindow::createActions()
{
    openUrlAct = new QAction(tr("&Open URL..."), this);
    openUrlAct->setShortcut(tr("Ctrl+U"));
    openUrlAct->setStatusTip(tr("Open a URL"));
    connect(openUrlAct, SIGNAL(triggered()), this, SLOT(openUrl()));
//! [1]

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setStatusTip(tr("Exit the application"));
    exitAct->setShortcuts(QKeySequence::Quit);
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

//! [2]
void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openUrlAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
}
//! [2]

//! [3]
void MainWindow::about()
{
    #ifdef PROXY_SUPPORT
    centralWidget->setAppProxy();
    #endif

    #ifdef APPLICATION_FOR_FORUM_PUBLIC
    QMessageBox::about(this, tr("About Kamigami SignTool"),
        tr("<html><head/><body><p align=\"center\">The <span style=\" font-weight:600; color:#ba008c;\">Kamigami SignTool</span> Version 0.90</p><p align=\"center\">Coded by <span style=\" font-weight:600; color:#4c4ce6;\">正义的带头大哥</span></p></body></html>"));
    #else
    QMessageBox::about(this, tr("About Kamigami SignTool"),
        tr("The <b>Kamigami SignTool</b> Version 0.90\n Coded by AsukaV_V"));
    #endif
}
//! [3]

//! [5]
void MainWindow::openUrl()
{
    bool ok;
    QString url = QInputDialog::getText(this, tr("Enter a URL"),
                  tr("URL:"), QLineEdit::Normal, "http://", &ok);

    if (ok && !url.isEmpty()) {
        centralWidget->webView->setUrl(url);
    }
}
//! [5]

//! [7]
void MainWindow::updateTextEdit()
{
    QWebFrame *mainFrame = centralWidget->webView->page()->mainFrame();
    QString frameText = mainFrame->toHtml();
    //centralWidget->plainTextEdit->setPlainText(frameText);
}

void MainWindow::WindowStateChangedProc(void)
{
    if (true == isMinimized()){
        //qDebug("Window Minimized: setHidden!");
        hide();
    }
}

void MainWindow::SystrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (QSystemTrayIcon::DoubleClick == reason){
        //qDebug() << "SystemTray double clicked: showNormal()!!";

        if (true == isHidden()){
            showNormal();
        }
        activateWindow();
    }
}

#if 0
void MainWindow::raiseWindow(Qt::ApplicationState state)
{
    qDebug() << "raiseWindow Slot";
    if (true == isHidden()){
        showNormal();
    }
    activateWindow();
}
#endif
//! [7]

//! [8]
void MainWindow::setStartupText()
{
//    QString string = "1. 在WebPagePreview中访问Kamigami论坛主页并登录帐号，登录状态会显示绿色的已登录。\n"
//                     "2. 登录状态变为已登录之后formhash会有显示数值，此时可以手动开始签到或根据设定时间自动开始签到。\n"
//                     "3. 登录状态变为已登录之后点击保存设置按钮会保存当前设置及登录账号信息，下次启动软件时候自动读取保存过的登录帐号信息和软件设置。\n"
//                     "4. 本地时间和论坛时间分别进行了显示，需要定时自动开始签到请参考论坛时间来设置自动开始签到时间，自动开始时间以本地时间为准。\n"
//                     "5. 最小化时隐藏窗口，鼠标左键双击系统托盘图标还原窗口显示。";
//    centralWidget->plainTextEdit->setPlainText(string);
}
//! [8]
