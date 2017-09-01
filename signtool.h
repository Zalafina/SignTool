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

#ifndef PREVIEWER_H
#define PREVIEWER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QWebFrame>
#include <QScriptEngine>

//#define PROXY_SUPPORT

#include "ui_signtool.h"

class MyNetworkCookie : public QNetworkCookieJar
{
    Q_OBJECT

public:
    MyNetworkCookie(QObject *parent = 0);
    ~MyNetworkCookie();

    QList<QNetworkCookie> getAllCookies();
    void setCookies(const QList<QNetworkCookie>& cookieList);
private:

};

class MainWindow;

//! [0]
class SignTool : public QWidget, public Ui::Form
{
    Q_OBJECT

public:
    SignTool(QWidget *parent = 0);

    void setNetworkManager(QNetworkAccessManager *networkMgr);
    #ifdef PROXY_SUPPORT
    void setAppProxy(void);
    #endif

    void setHomeUrl(const QUrl &url);
    static QUrl getHomeUrl(void);

    void postRequest(const QNetworkRequest &post_req, const QByteArray &post_param);

    void clearSavedRequestData(void);
    bool readformhash(QString &inputText);
    bool readTimeStatus(QString &inputText);

    void repeatSignStart(void);
    void repeatSignStop(void);

    void postSignData(void);

    void checkAndDoShutdown(void);

	void saveCookie(void);
    void loadCookie(void);

    void saveSettings(void);
    void loadSettings(void);

    void statusGetProc(void);

protected:
    void timerEvent(QTimerEvent *event);

private slots:
    void cycleTimerTimeout(void);
    void repeatSignInTimerTimeout(void);

    void SignPostReplyFinished(QNetworkReply *reply);
    void StatusGetReplyFinished(QNetworkReply *reply);
    void WebViewLoadFinished(bool ok);
    void formhashGetReplyFinished(QNetworkReply *reply);

private slots:
    void on_homeButton_clicked();

    void on_signButton_toggled(bool checked);

    void on_formhashData_textChanged(const QString &arg1);

    void on_todaysayTextEdit_textChanged();

    void on_startTimecheckBox_clicked(bool checked);

    void on_starttimeEdit_timeChanged(const QTime &time);

    void on_endtimeEdit_timeChanged(const QTime &time);

    void on_saveSettingButton_clicked();

private:
    void init_qdxqMap(void);

private:
    static QUrl m_homeUrl;
public:
    QNetworkAccessManager *m_WebPageNetworkAccessMgr;
    QNetworkAccessManager *m_SignPostNetworkAccessMgr;
    QNetworkAccessManager *m_StatusGetNetworkAccessMgr;
    QNetworkAccessManager *m_formhashGetNetworkAccessMgr;
    MyNetworkCookie *m_CookieJar;
    QNetworkReply *m_PostReply;

private:
    MainWindow *mainWindow;
    bool signInRepeat;
    bool loginStatus;
    QNetworkRequest m_SavedPostRequest;
    QByteArray      m_SavedPostParam;
    quint32         m_SendCount;
    quint32         m_ReplyCount;
    QHash<int,QByteArray>   m_qdxqMap;
    QTimer *m_Timer;
    QTimer *m_RepeatSignInTimer;
};
//! [0]

#endif
