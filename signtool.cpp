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
#include "mainwindow.h"
#include "signtool.h"

QUrl SignTool::m_homeUrl(QUrl("http://www.kamigami.org/plugin.php?id=dsu_paulsign:sign"));
static const QUrl GET_STATUS_URL(QUrl("http://www.kamigami.org/home.php?mod=spacecp&ac=profile&op=password"));
static const QUrl SIGN_IN_URL(QUrl("http://www.kamigami.org/plugin.php?id=dsu_paulsign:sign&operation=qiandao&infloat=1&inajax=1"));
static const QString LOGIN_TEXT_STR (QString("<html><head/><body><p><span style=\" font-weight:600; color:#00ff00;\">已登录</span></p></body></html>"));
static const QString LOGOUT_TEXT_STR(QString("<html><head/><body><p><span style=\" font-weight:600; color:#ff0000;\">未登录</span></p></body></html>"));

#ifdef APPLICATION_FOR_FORUM_PUBLIC
static const int SIGN_RETRY_TIMER = 25000;
#else
static const int SIGN_RETRY_TIMER = 5;
#endif
static const int TODAY_SAY_STR_MAX = 200;
static const int CYCLE_TIMER_SETTING = 1000;
static const int CHECK_FORUM_STATUS_TIMEOUT = 20000;

MyNetworkCookie::MyNetworkCookie(QObject *parent) :
  QNetworkCookieJar(parent)
{

}

MyNetworkCookie::~MyNetworkCookie()
{

}

QList<QNetworkCookie> MyNetworkCookie::getAllCookies(void)
{
    return allCookies();
}

void MyNetworkCookie::setCookies(const QList<QNetworkCookie> &cookieList)
{
    setAllCookies(cookieList);
}

//! [0]
SignTool::SignTool(QWidget *parent)
    : QWidget(parent)
  , m_WebPageNetworkAccessMgr(NULL)
  , m_SignPostNetworkAccessMgr(NULL)
  , m_StatusGetNetworkAccessMgr(NULL)
  , m_formhashGetNetworkAccessMgr(NULL)
  , m_CookieJar(NULL)
  , m_PostReply(NULL)
  , mainWindow(dynamic_cast<MainWindow *>(parent))
  , signInRepeat(false)
  , loginStatus(false)
  , m_SavedPostRequest()
  , m_SavedPostParam()
  , m_SendCount(0)
  , m_ReplyCount(0)
  , m_qdxqMap()
  , m_Timer(NULL)
  , m_RepeatSignInTimer(NULL)
{
    setupUi(this);

    init_qdxqMap();

    m_Timer = new QTimer(this);
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(cycleTimerTimeout()));
    m_Timer->start(CYCLE_TIMER_SETTING);

    m_RepeatSignInTimer = new QTimer(this);
    connect(m_RepeatSignInTimer, SIGNAL(timeout()), this, SLOT(repeatSignInTimerTimeout()));

    m_CookieJar = new MyNetworkCookie();

    m_SignPostNetworkAccessMgr = new QNetworkAccessManager();
    m_StatusGetNetworkAccessMgr = new QNetworkAccessManager();
    m_formhashGetNetworkAccessMgr = new QNetworkAccessManager();
}

void SignTool::setNetworkManager(QNetworkAccessManager *networkMgr)
{
    Q_ASSERT(networkMgr != NULL);

    if (networkMgr != NULL){
        m_WebPageNetworkAccessMgr = networkMgr;

        m_WebPageNetworkAccessMgr->setCookieJar(m_CookieJar);

        loadCookie();

        m_SignPostNetworkAccessMgr->setCookieJar(m_CookieJar);
        m_StatusGetNetworkAccessMgr->setCookieJar(m_CookieJar);
        m_formhashGetNetworkAccessMgr->setCookieJar(m_CookieJar);

        connect(m_SignPostNetworkAccessMgr,SIGNAL(finished(QNetworkReply*)), this, SLOT(SignPostReplyFinished(QNetworkReply*)));
        connect(m_StatusGetNetworkAccessMgr,SIGNAL(finished(QNetworkReply*)), this, SLOT(StatusGetReplyFinished(QNetworkReply*)));
        connect(m_formhashGetNetworkAccessMgr,SIGNAL(finished(QNetworkReply*)), this, SLOT(formhashGetReplyFinished(QNetworkReply*)));

        connect(webView, SIGNAL(loadFinished(bool)), this, SLOT(WebViewLoadFinished(bool)));
        //loadFinished
    }
}

#ifdef PROXY_SUPPORT
void SignTool::setAppProxy(void)
{
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName("xxx.xxx.xxx");
    proxy.setPort(8080);
    proxy.setUser("xxx");
    proxy.setPassword("xxxxx");
    QNetworkProxy::setApplicationProxy(proxy);
}
#endif

void SignTool::setHomeUrl(const QUrl &url)
{
    m_homeUrl = url;
}

QUrl SignTool::getHomeUrl(void)
{
    return m_homeUrl;
}

void SignTool::postRequest(const QNetworkRequest &post_req, const QByteArray &post_param)
{
    m_PostReply = m_SignPostNetworkAccessMgr->post( post_req , post_param );
    //m_PostReply = m_WebPageNetworkAccessMgr->post( post_req , post_param );

    m_SavedPostRequest = post_req;
    m_SavedPostParam = post_param;

    //connect(m_PostReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(requestError(QNetworkReply::NetworkError)));
}

void SignTool::clearSavedRequestData(void)
{
    m_SavedPostParam.clear();
    m_SavedPostRequest = QNetworkRequest();
}

bool SignTool::readformhash(QString &inputText)
{
    QString formhash = "formhash=";
    int index = inputText.indexOf(formhash);

    bool result = true;
    if (-1 == index){
        result = false;
    }
    else{
        QString hashnum;

        int hashstartIndex = index + formhash.length();

        for (int loopCnt = hashstartIndex; loopCnt < (hashstartIndex + 8); loopCnt++){
            hashnum.append(inputText.at(loopCnt));
        }

        if (formhashData->text() != hashnum){
            formhashData->setText(hashnum);
        }

        //qDebug() << "formhash is " << hashnum;

        result = true;
    }

    return result;
}

bool SignTool::readTimeStatus(QString &inputText)
{
    QString timeStr = QString("GMT+8, ");
    int startIndex = inputText.indexOf(timeStr);

    bool result = true;
    if (-1 == startIndex){
        result = false;
    }
    else{
        QString forumDateTime;

        int endIndex = inputText.indexOf('\n', startIndex);

        int startPosition = startIndex + timeStr.length();
        forumDateTime = inputText.mid(startPosition, (endIndex - startPosition));

        //qDebug() << "forumDateTime is:" << forumDateTime;
        QString forumTime = forumDateTime.section(' ', 1, 1);
        //qDebug() << "forumTime is:" << forumTime;

        static QString lastForumTime;

        #ifdef APPLICATION_FOR_FORUM_PUBLIC
        #else
        if (Qt::Checked == startTimecheckBox->checkState()){
            if ((lastForumTime != QString("23:59"))
                && (forumTime == QString("23:59"))){
                if (signInRepeat != true){
                    if (false == formhashData->text().isEmpty()){
                        if (true == mainWindow->isHidden()){
                            QString titlec=tr("Kamigami签到工具");
                            QString textc("已自动开始签到！");
                            mainWindow->get_SysTrayIcon()->showMessage(titlec,textc,QSystemTrayIcon::Information, 3600000);
                        }

                        repeatSignStart();
                    }
                }
            }
        }
        #endif

        lastForumTime = forumTime;

        forumTime.prepend(QString("<b><font color=\"#860086\">"));
        forumTime.append(QString("</font></b>"));

        forumTimelabel->setText(forumTime);

        result = true;
    }

    return result;
}

void SignTool::repeatSignStart(void)
{
    if (signInRepeat != true){
        signButton->setText(QString("停止签到"));

        #ifdef APPLICATION_FOR_FORUM_PUBLIC
        QTime curTime;
        curTime= QTime::currentTime();
        qsrand(curTime.msec()+curTime.second()*1000);
        int randomNum = qrand();
        randomNum = randomNum % 10;
        qDebug("randomNum is %d", randomNum);

        int retryTimer = SIGN_RETRY_TIMER + (randomNum * 1000);
        qDebug("retryTimer is %d", retryTimer);
        m_RepeatSignInTimer->start(retryTimer);
        #else
        //m_RepeatSignInTimer->start(SIGN_RETRY_TIMER);
        qDebug("Resend Timer is %dms", ResendTimespinBox->value());
        m_RepeatSignInTimer->start(ResendTimespinBox->value());
        #endif

        qDebug("RepeatSignIn set to TRUE!!!");
        signInRepeat = true;
    }
}

void SignTool::repeatSignStop(void)
{
    if (signInRepeat != false){
        m_RepeatSignInTimer->stop();
        clearSavedRequestData();
        //formhashData->clear();
        m_SendCount = 0;
        m_ReplyCount = 0;
        retryCount->display(static_cast<int>(m_SendCount));
        signButton->setText(QString("开始签到"));

        qDebug("RepeatSignIn set to FALSE");
        signInRepeat = false;
    }
}

void SignTool::postSignData(void)
{
    QNetworkRequest request;

    //设置url
    request.setUrl(SIGN_IN_URL);

    QByteArray param;
    // 设置formhash
    param.append("formhash=");
    param.append(formhashData->text().toLatin1());
    param.append("&");

    QString curTodaySay = todaysayTextEdit->toPlainText();

    if (false == curTodaySay.isEmpty()){
        // 设置签到心情
        QByteArray xqParam = m_qdxqMap.value(qdxqComboBox->currentIndex(), QByteArray("kx"));
        param.append("qdxq=");
        param.append(xqParam);
        param.append("&");

        // 设置签到模式为自己填写签到内容
        param.append("qdmode=1&");

        // 设置我今天最想说的内容(GBK编码)
        QTextCodec *textCodeC = QTextCodec::codecForName("GBK");
        QTextEncoder *textEncoder = textCodeC->makeEncoder();
        QByteArray convertData = textEncoder->fromUnicode(curTodaySay);
        param.append("todaysay=");
        param.append(convertData);
    }
    else{
        // 设置签到心情
        QByteArray xqParam = m_qdxqMap.value(qdxqComboBox->currentIndex(), QByteArray("kx"));
        param.append("qdxq=");
        param.append(xqParam);
        param.append("&");
        param.append("qdmode=2&");
        param.append("fastreply=0");
    }

    //qDebug("Sign Post Parameter is (%s)", param.constData());

    //设置头信息
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::ContentLengthHeader, param.length());

    postRequest(request, param);
}

void SignTool::checkAndDoShutdown(void)
{
    if ((true == shutdowncheckBox->isEnabled())
        && (Qt::Checked == shutdowncheckBox->checkState())){
        QStringList environment = QProcess::systemEnvironment();

        QRegularExpression regExp("^SYSTEMROOT=(.+)");

        int searchIndex = environment.indexOf(regExp);

        QString shutdownProgram;
        if (searchIndex != -1){
            QString systemRoot = environment.at(searchIndex);
            QString preString("SYSTEMROOT=");
            QString systemRootPath = systemRoot.right(systemRoot.size() - preString.size());
            shutdownProgram.append(systemRootPath);
            shutdownProgram.append(QString("/system32/shutdown.exe"));

            shutdownProgram.replace(QChar('\\'), QChar('/'));

            qDebug() << "searchIndex is " << searchIndex;
        }
        else{
            shutdownProgram = QString("C:/Windows/system32/shutdown.exe");
        }

        qDebug() << "shutdownProgram is " << shutdownProgram;

        QStringList arguments;
        arguments << "-s";
        QProcess *myProcess = new QProcess();
        myProcess->start(shutdownProgram, arguments);
    }
}

void SignTool::saveCookie(void)
{
    QList<QNetworkCookie> cookieList = m_CookieJar->getAllCookies();

    QSettings cookieSettings(QString("settings.ini"), QSettings::IniFormat);

    QByteArray cookieData;
    QNetworkCookie tmp_cookie;
    Q_FOREACH (tmp_cookie, cookieList){
        cookieData.append(tmp_cookie.toRawForm());
        cookieData.append("\n");
    }

    cookieData.remove((cookieData.size()-1), 1);

    qDebug() << cookieData;
    cookieSettings.setValue(QLatin1String("cookies"), cookieData);
}

void SignTool::loadCookie(void)
{
    QSettings cookieSettings(QString("settings.ini"), QSettings::IniFormat);

    if (true == cookieSettings.contains("cookies")){
        QByteArray cookieData = cookieSettings.value("cookies").toByteArray();
        QList<QNetworkCookie> cookieList = QNetworkCookie::parseCookies(cookieData);

//        qDebug() << "Loaded cookieList size is:" << cookieList.size();
//        QNetworkCookie tmp_cookie;
//        Q_FOREACH (tmp_cookie, cookieList){
//            qDebug("%s", tmp_cookie.toRawForm().constData());
//        }

        m_CookieJar->setCookies(cookieList);
    }
}

void SignTool::saveSettings(void)
{
    QSettings generalSettings(QString("settings.ini"), QSettings::IniFormat);

//    if (formhashData->text().size() > 0){
//        generalSettings.setValue(QString("formhash"), formhashData->text());
//    }

    generalSettings.setValue(QString("qdxq"), qdxqComboBox->currentIndex());
    generalSettings.setValue(QString("todaysay"), todaysayTextEdit->toPlainText());

    if (Qt::Checked == startTimecheckBox->checkState()){
        generalSettings.setValue(QString("autostart"), true);
    }
    else{
        generalSettings.setValue(QString("autostart"), false);
    }

    generalSettings.setValue(QString("starttime"), starttimeEdit->time());
    generalSettings.setValue(QString("endtime"), endtimeEdit->time());

    if (Qt::Checked == shutdowncheckBox->checkState()){
        generalSettings.setValue(QString("autoshutdown"), true);
    }
    else{
        generalSettings.setValue(QString("autoshutdown"), false);
    }

    generalSettings.setValue(QString("resendtimer"), ResendTimespinBox->value());
}

void SignTool::loadSettings(void)
{
    QSettings generalSettings(QString("settings.ini"), QSettings::IniFormat);

//    if (true == generalSettings.contains("cookies")){
//        if (true == generalSettings.contains("formhash")){
//            QString formhash = generalSettings.value("formhash").toByteArray();
//            if (formhash.size() > 0){
//                formhashData->setText(formhash);
//            }
//            else{
//            }
//        }
//    }

    if (true == generalSettings.contains("qdxq")){
        int qdxqIndex = generalSettings.value("qdxq").toInt();
        qdxqComboBox->setCurrentIndex(qdxqIndex);
    }

    if (true == generalSettings.contains("todaysay")){
        QString todaySayStr = generalSettings.value("todaysay").toByteArray();
        todaysayTextEdit->setPlainText(todaySayStr);
    }

    if (true == generalSettings.contains("autostart")){
        bool autoStart = generalSettings.value("autostart").toBool();
        if (true == autoStart){
            startTimecheckBox->setCheckState(Qt::Checked);
        }
        else{
            startTimecheckBox->setCheckState(Qt::Unchecked);
        }
    }

    if (true == generalSettings.contains("starttime")){
        QTime startTime = generalSettings.value("starttime").toTime();
        starttimeEdit->setTime(startTime);
    }

    if (true == generalSettings.contains("endtime")){
        QTime endTime = generalSettings.value("endtime").toTime();
        endtimeEdit->setTime(endTime);
    }

    if (true == generalSettings.contains("autoshutdown")){
        bool autoShutdown = generalSettings.value("autoshutdown").toBool();
        if (true == autoShutdown){
            shutdowncheckBox->setCheckState(Qt::Checked);
        }
        else{
            shutdowncheckBox->setCheckState(Qt::Unchecked);
        }
    }

    if (true == generalSettings.contains("resendtimer")){
        int resendTimer = generalSettings.value("resendtimer").toInt();
        ResendTimespinBox->setValue(resendTimer);
    }
}

void SignTool::statusGetProc(void)
{
    QNetworkRequest request;
    request.setUrl(GET_STATUS_URL);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/59.0.3071.109 Safari/537.36");

    m_StatusGetNetworkAccessMgr->get(request);
}

void SignTool::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
}

void SignTool::cycleTimerTimeout(void)
{
    static quint8 checkStatusCount = 0;

    checkStatusCount += 1;
    if (checkStatusCount >= (CHECK_FORUM_STATUS_TIMEOUT/CYCLE_TIMER_SETTING)){
        #ifdef APPLICATION_FOR_FORUM_PUBLIC
        statusGetProc();
        #else
//        if (signInRepeat != true){
//            webView->reload();
//        }

        statusGetProc();
        #endif

        checkStatusCount = 0;
    }

    QString curTimeStr = QTime::currentTime().toString("HH:mm:ss");

    if (Qt::Checked == startTimecheckBox->checkState()){
        if (starttimeEdit->time() <= QTime::currentTime()
            && ((QTime::currentTime() <= endtimeEdit->time())
                || (0 == endtimeEdit->time().hour()))){
            curTimeStr.prepend(QString("<b><font color=\"#ff0000\">"));
            curTimeStr.append(QString("</font></b>"));
        }

        if (starttimeEdit->text() == QTime::currentTime().toString("HH:mm")){
            if (signInRepeat != true){
                if (false == formhashData->text().isEmpty()){
                    if (true == mainWindow->isHidden()){
                        QString titlec=tr("Kamigami签到工具");
                        QString textc("已自动开始签到！");
                        mainWindow->get_SysTrayIcon()->showMessage(titlec,textc,QSystemTrayIcon::Information, 3600000);
                    }

                    repeatSignStart();
                }
                else{
                    QString string = "formhash未取得, 请登录并点击<b><font color=\"#ff0000\">gethash并保存设置按钮</font></b>取得formhash";
                    plainTextEdit->setPlainText(string);
                }
            }
        }

        if (endtimeEdit->text() == QTime::currentTime().toString("HH:mm")){
            if (signInRepeat != false){
                repeatSignStop();

                checkAndDoShutdown();
            }
        }
    }

    curTimelabel->setText(curTimeStr);
}

void SignTool::repeatSignInTimerTimeout(void)
{
    if (false == formhashData->text().isEmpty()){
        postSignData();

        m_SendCount += 1;
        if (m_SendCount > 99999999){
            m_SendCount = 0;
        }
        retryCount->display(static_cast<int>(m_SendCount));
    }
    else{
        qDebug("Repeat Sign In TimeOut formhash is Empty!");
    }
}

void SignTool::SignPostReplyFinished(QNetworkReply *reply)
{
    QVariant status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    //qDebug("SignPostReplyFinished HttpStatusCode: %d", status_code.toInt());

    if (200 == status_code.toInt()){
        m_ReplyCount += 1;

        if (m_ReplyCount > 99999999){
            m_ReplyCount = 0;
        }
    }

    if(reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll();  //Read All Bytes from postRequest Reply
        QTextCodec *detectCodeC = QTextCodec::codecForHtml(bytes, QTextCodec::codecForName("GBK"));
        QString result = detectCodeC->toUnicode(bytes.constData(), bytes.size());
        QString signSuccess = QString("恭喜你签到成功");

        if (true == result.contains(signSuccess)){
            plainTextEdit->setPlainText(result);
        }
    }
    else{
        qDebug() << "SignPostReplyFinished with Error:  " << reply->error() << " " <<reply->errorString();
    }

    //Note: After the request has finished, it is the responsibility of the user to delete the QNetworkReply object
    // at an appropriate time. Do not directly delete it inside the slot connected to finished().
    // You can use the deleteLater() function.
    reply->deleteLater();
}

void SignTool::StatusGetReplyFinished(QNetworkReply *reply)
{
    QVariant status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    //qDebug("StatusGet Reply HttpStatusCode: %d", status_code.toInt());

    if(reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll();  //Read All Bytes from postRequest Reply

        QTextCodec *detectCodeC = QTextCodec::codecForHtml(bytes, QTextCodec::codecForName("GBK"));
        //qDebug("detectCodeC name is (%s)", detectCodeC->name().constData());

        //detectCodeC = QTextCodec::codecForName("GBK");
        QString result = detectCodeC->toUnicode(bytes.constData(), bytes.size());

        //plainTextEdit->setPlainText(result);

        QString loginOutStatus = QString("您需要先登录才能继续本操作");
        QString loginInStatus = QString("您必须填写原密码才能修改下面的资料");

        bool readTimeResult = readTimeStatus(result);
        Q_UNUSED(readTimeResult);

        if (true == result.contains(loginOutStatus)){
            loginStatuslabel->setText(LOGOUT_TEXT_STR);
            loginStatus = false;
            formhashData->clear();

            //qDebug() << "StatusGetReply get Logout Status";
        }
        else if (true == result.contains(loginInStatus)){
            loginStatuslabel->setText(LOGIN_TEXT_STR);
            loginStatus = true;

            bool readresult = readformhash(result);
            Q_UNUSED(readresult);

            //qDebug() << "StatusGetReply get LogIn Status";
        }
        else{
            loginStatuslabel->setText(LOGOUT_TEXT_STR);
            loginStatus = false;
            formhashData->clear();

            qDebug() << "StatusGetReply Login Status Error!";
        }
    }
    else if ((503 == status_code.toInt())
             && (QNetworkReply::ServiceUnavailableError == reply->error())){
        QByteArray bytes = reply->readAll();  //Read All Bytes from postRequest Reply

        QTextCodec *detectCodeC = QTextCodec::codecForHtml(bytes, QTextCodec::codecForName("GBK"));
        //qDebug("detectCodeC name is (%s)", detectCodeC->name().constData());

        //detectCodeC = QTextCodec::codecForName("GBK");
        QString result = detectCodeC->toUnicode(bytes.constData(), bytes.size());

        //plainTextEdit->setPlainText(result);

        QString checkingStatus = QString("浏览器安全检查中");

        if (true == result.contains(checkingStatus)){
            qDebug() << "StatusGetReply: (503)Security Checking";

            QString positionStr("var s,t,o,p,b,r,e,a,k,i,n,g,f, ");
            if (true == result.contains(positionStr)){
                plainTextEdit->setPlainText(result);

                int start_position = result.indexOf(positionStr);
                start_position = start_position + positionStr.length();
                int end_position = result.indexOf("\n", start_position);
                QStringRef strRef = result.midRef(start_position, (end_position - start_position));
                qDebug("strRef:%s(s:%d, e:%d)", strRef.toLatin1().constData(), start_position, end_position);

                int var_name_pos = strRef.indexOf("=");
                QStringRef var_name_a = strRef.left(var_name_pos);
                int mark_start_pos = strRef.indexOf("\"", var_name_a.length());
                int mark_end_pos = strRef.indexOf("\"", (mark_start_pos + 1));
                QStringRef var_name_b = strRef.mid((mark_start_pos + 1), (mark_end_pos - (mark_start_pos + 1)));

                QString var_name = var_name_a.toString() + "." + var_name_b.toString();

                QScriptEngine myEngine;
                myEngine.evaluate(strRef.toString());
                QScriptValue final_value = myEngine.evaluate(var_name);
                qint32 value_int = final_value.toInt32();
                qDebug("%s:%d", var_name.toLatin1().constData(), value_int);

                int start_position_b = result.indexOf(var_name, end_position);
                int end_position_b = result.indexOf("\n", start_position_b);
                QStringRef strRef_b = result.midRef(start_position_b, (end_position_b - start_position_b));
                QString str_b = strRef_b.toString();
                str_b.replace("a.value", "a_value");
                str_b.replace("t.length", "t_length");
                qDebug("str_b:\n%s\n(s:%d, e:%d)", str_b.toLatin1().constData(), start_position_b, end_position_b);

                myEngine.evaluate("var a_value;");
                myEngine.evaluate("var t_length = 16;");
                myEngine.evaluate(str_b);
                final_value = myEngine.evaluate(var_name);
                value_int = final_value.toInt32();
                qDebug("%s:%d", var_name.toLatin1().constData(), value_int);
            }
            else{
                qDebug() << "StatusGetReply: (503) positionStr does not contains!!!";
            }
        }
        else{
            qDebug() << "StatusGetReply: (503) Other Error!!!";
        }
    }
    else{
        qDebug("StatusGet Reply HttpStatusCode: %d", status_code.toInt());
        qDebug() << "StatusGetReplyFinished with Error:  " << reply->error() << ":" << (int)(reply->error()) <<reply->errorString();
        m_StatusGetNetworkAccessMgr->clearAccessCache();
    }

    reply->deleteLater();
}

void SignTool::WebViewLoadFinished(bool ok)
{
    if (ok == true){
        QString plantextString;
        QString htmlString;

        plantextString = webView->page()->currentFrame()->toPlainText();
        htmlString = webView->page()->currentFrame()->toHtml();

        //plainTextEdit->setPlainText(plantextString);
        //plainTextEdit->setPlainText(htmlString);

        QString loginOutStatus_1 = QString("您需要先登录才能继续本操作");

        QString loginOutStatus_2a = QString("自动登录");
        QString loginOutStatus_2b = QString("注册");
        QString loginOutStatus_2c = QString("用户名");
        QString loginOutStatus_2d = QString("密码");

        QString loginInStatus = userName->displayText() + QString(" 在线");

        //plainTextEdit->setPlainText(loginInStatus);

        bool readTimeResult = readTimeStatus(plantextString);
        Q_UNUSED(readTimeResult);

        if (QString() == userName->displayText()){
            loginStatuslabel->setText(LOGOUT_TEXT_STR);
            loginStatus = false;
            formhashData->clear();
            qDebug() << "WebViewLoadFinished Empty UserName.";
        }
        else if (true == plantextString.contains(loginInStatus)){
            loginStatuslabel->setText(LOGIN_TEXT_STR);
            loginStatus = true;

            bool readresult = readformhash(htmlString);
            Q_UNUSED(readresult);

            qDebug() << "WebViewLoadFinished UserName Online contains.";
        }
        else if (true == plantextString.contains(loginOutStatus_1)){
            loginStatuslabel->setText(LOGOUT_TEXT_STR);
            loginStatus = false;
            formhashData->clear();

            qDebug() << "WebViewLoadFinished loginOutStatus 1 contains.";
        }
        else if ((true == plantextString.contains(loginOutStatus_2a))
            && (true == plantextString.contains(loginOutStatus_2b))
            && (true == plantextString.contains(loginOutStatus_2c))
            && (true == plantextString.contains(loginOutStatus_2d))){
            loginStatuslabel->setText(LOGOUT_TEXT_STR);
            loginStatus = false;
            formhashData->clear();

            qDebug() << "WebViewLoadFinished loginOutStatus 2 contains.";
        }
//        else{
//            loginStatuslabel->setText(LOGOUT_TEXT_STR);
//            loginStatus = false;
//            formhashData->clear();

//            qDebug() << "StatusGetReply Login Status Error!";
//        }
    }
    else{
        qDebug("WebViewLoad Failed");
    }
}

void SignTool::formhashGetReplyFinished(QNetworkReply *reply)
{
    QVariant status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    qDebug("GetHash Reply HttpStatusCode: %d", status_code.toInt());

    if(reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll();  //Read All Bytes from postRequest Reply

        QTextCodec *detectCodeC = QTextCodec::codecForHtml(bytes, QTextCodec::codecForName("GBK"));
        qDebug("detectCodeC name is (%s)", detectCodeC->name().constData());

        //detectCodeC = QTextCodec::codecForName("GBK");
        QString result = detectCodeC->toUnicode(bytes.constData(), bytes.size());
        QString loginStatus = QString("您需要先登录才能继续本操作");

        if (false == result.contains(loginStatus)){
            bool readresult = readformhash(result);
            if (true == readresult){
                saveCookie();
                saveSettings();

                QMessageBox::information(this, tr("Kamigami SignTool"), tr("登录信息及设置保存成功！"));
            }
        }
        else{
            formhashData->clear();
            QMessageBox::warning(this, tr("Kamigami SignTool"), tr("请先登录并点击<b><font color=\"#ff0000\">gethash并保存设置按钮</font></b>取得formhash"));
        }

        qDebug()<<"formhashGetReply Start:";
        qDebug()<<result;
    }
    else{
        qDebug() << "formhashGetReplyFinished with Error:  " << reply->error() << " " <<reply->errorString();
    }

    reply->deleteLater();
}

void SignTool::on_homeButton_clicked()
{
    webView->setUrl(SignTool::getHomeUrl());
}

void SignTool::on_signButton_toggled(bool checked)
{
    if (true == checked){
        if (false == formhashData->text().isEmpty()){
            repeatSignStart();
        }
        else{
            QMessageBox::warning(this, tr("Kamigami SignTool"), tr("formhash未取得, 请先登录论坛帐号!"));
            qDebug("formhash is Empty!!!");
        }
    }
    else{
        repeatSignStop();
    }
}

void SignTool::on_formhashData_textChanged(const QString &arg1)
{
    if (false == arg1.isEmpty()){
        signButton->setEnabled(true);
    }
    else{
        signButton->setEnabled(false);
    }
}

void SignTool::on_todaysayTextEdit_textChanged()
{
    QString curTodaySay = todaysayTextEdit->toPlainText();

    if (curTodaySay.length() > TODAY_SAY_STR_MAX){
        curTodaySay.resize(TODAY_SAY_STR_MAX);
        todaysayTextEdit->setPlainText(curTodaySay);
        //qDebug() << "Today Say resize to : " << curTodaySay;
    }
}

void SignTool::init_qdxqMap(void)
{
    m_qdxqMap.insert( 0,    QByteArray("kx"));      // 开心
    m_qdxqMap.insert( 1,    QByteArray("ng"));      // 难过
    m_qdxqMap.insert( 2,    QByteArray("ym"));      // 郁闷
    m_qdxqMap.insert( 3,    QByteArray("wl"));      // 无聊
    m_qdxqMap.insert( 4,    QByteArray("nu"));      // 怒
    m_qdxqMap.insert( 5,    QByteArray("ch"));      // 擦汗
    m_qdxqMap.insert( 6,    QByteArray("fd"));      // 奋斗
    m_qdxqMap.insert( 7,    QByteArray("yl"));      // 慵懒
    m_qdxqMap.insert( 8,    QByteArray("shuai"));   // 衰
}

void SignTool::on_startTimecheckBox_clicked(bool checked)
{
    if (true == checked){
        starttimeEdit->setEnabled(true);
        endtimeEdit->setEnabled(true);
        shutdowncheckBox->setEnabled(true);
    }
    else{
        starttimeEdit->setEnabled(false);
        endtimeEdit->setEnabled(false);
        shutdowncheckBox->setEnabled(false);
    }
}

void SignTool::on_starttimeEdit_timeChanged(const QTime &time)
{
    Q_UNUSED(time);

    if (starttimeEdit->time() == endtimeEdit->time()){
        int minuteSet = 0;
        int hourSet = 0;

        if (starttimeEdit->time().minute() >= 58){
            if (starttimeEdit->time().hour() >= 23){
                hourSet = 0;
            }
            else{
                hourSet = starttimeEdit->time().hour() + 1;
            }

            minuteSet = starttimeEdit->time().minute() + 2 - 60;
        }
        else{
            hourSet = starttimeEdit->time().hour();
            minuteSet = starttimeEdit->time().minute() + 2;
        }

        endtimeEdit->setTime(QTime(hourSet, minuteSet));

        qDebug() << "starttimeEdit_timeChanged settingEndTime is" << QTime(hourSet, minuteSet);
    }
}

void SignTool::on_endtimeEdit_timeChanged(const QTime &time)
{
    Q_UNUSED(time);

    if (starttimeEdit->time() == endtimeEdit->time()){
        int minuteSet = 0;
        int hourSet = 0;

        if (starttimeEdit->time().minute() >= 58){
            if (starttimeEdit->time().hour() >= 23){
                hourSet = 0;
            }
            else{
                hourSet = starttimeEdit->time().hour() + 1;
            }

            minuteSet = starttimeEdit->time().minute() + 2 - 60;
        }
        else{
            hourSet = starttimeEdit->time().hour();
            minuteSet = starttimeEdit->time().minute() + 2;
        }

        endtimeEdit->setTime(QTime(hourSet, minuteSet));

        qDebug() << "endtimeEdit_timeChanged settingEndTime is" << QTime(hourSet, minuteSet);
    }
}

void SignTool::on_saveSettingButton_clicked()
{
    if (true == loginStatus){
        saveCookie();
        saveSettings();
        QMessageBox::information(this, tr("Kamigami SignTool"), tr("登录信息及设置保存成功！"));
    }
    else{
        QMessageBox::warning(this, tr("Kamigami SignTool"), tr("请先登录再保存设置，以便保存账户登录信息！"));
    }
}
