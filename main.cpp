#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>

#include <iostream>
#include <cstdarg>

inline void fixFileName(QDir dir)
{
    for (const auto &it : dir.entryList(QDir::Files)){
        if (it.right(4) != ".svg")
            continue;
        QString newName = it;
        newName.remove(newName.length() - 4, 4);
        if (newName.size() > 4)
            if (newName[0].isDigit() && newName[2].isDigit() && newName[3] == '-')
                newName.remove(0, 4);
        newName.replace('-', '_').replace(' ', '_');
        if (newName + ".svg" == it)
            continue;

        int newI = 0;
        while (true){
            QString posStr = "";
            if (newI != 0)
                posStr = "_" + QString::number(newI);
            qDebug() << newName.toLower() + posStr + ".svg";
            if (dir.entryList(QDir::Files).contains(newName.toLower() + posStr + ".svg"))
                newI++;
            else {
                newName += posStr;
                break;
            }
        }

        qDebug() << "Rename" << dir.absoluteFilePath(it) << QFile::rename(it, newName.replace('-', '_').replace('.', '_').toLower() + ".svg");
        continue;
        qDebug() << "\n------------------------------------------------------------------------------------------------";
        qDebug() << "Handle file name:\n\tfrom " << dir.absoluteFilePath(it) << "\n\tto   "
                 << dir.absoluteFilePath(newName.replace('-', '_').replace(' ', '_')).toLower() + ".svg";
        QFile file(dir.absoluteFilePath(it));
        qDebug() << "exists :" << file.exists();
        qDebug() << "rename :" << file.rename(dir.absoluteFilePath(newName.replace('-', '_').replace(' ', '_')).toLower() + ".svg");
        qDebug() << "error  :" << file.error() << "[" << file.errorString() << "]\n";
        qDebug() << "copy       :" << file.copy(dir.absoluteFilePath(newName.replace('-', '_').replace(' ', '_')).toLower() + ".svg");
        qDebug() << "error      :" << file.errorString() << file.error();
        qDebug() << "new name   :" << file.fileName();
        qDebug() << "------------------------------------------------------------------------------------------------\n\n";
    }
}

inline void rmdir(QDir dir, QString subDir)
{
    QDir subdir(dir);
    if (! subdir.cd(subDir))
        return;
    qDebug() << subdir.path() << subDir;
    for (const auto &i : subdir.entryList(QDir::Files))
        subdir.remove(i);
    for (const auto &i : subdir.entryList(QDir::Dirs | QDir::NoDot | QDir::NoDotDot))
        rmdir(subdir, i);

    dir.rmdir(subDir);
}

inline void createCmakeFiles(QDir dir, QStringList dirList)
{
    // update resource.qbs
    QFile file(dir.absoluteFilePath("CMakeLists.txt"));
    if (file.open(QIODevice::Truncate | QIODevice::ReadWrite)){
        QTextStream stream(&file);
        stream << "set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib)\n\n";
        for (const QString &it : dirList)
            stream << "add_subdirectory(" << it << ")\n";
        file.close();
    }
    else
        qDebug() << "cmake not rewrite";

    for (const auto &it : dirList){
        QDir rcDir(dir.path() + "/" + it);
        fixFileName(rcDir);

        // create $ cmake
        QFile itFile(rcDir.absoluteFilePath("CMakeLists.txt"));
        if (itFile.open(QIODevice::Truncate | QIODevice::ReadWrite)){
            QTextStream stream(&itFile);
            stream << "cmake_minimum_required(VERSION 3.19)\n"
                      "project(rc_" << it << " VERSION 1.0.0)\n"
                      "\n"
                   << "set (CMAKE_AUTORCC ON)\n"
                      "set (CMAKE_CXX_STANDARD 20)\n"
                      "find_package(Qt6Core)\n"
                      "\n"
                      "add_library(rc_" << it << " rc_" << it << ".qrc)\n"
                      "target_link_libraries(rc_" << it << " Qt6::Core)";
            itFile.close();
        }
        else
            qDebug() << "cmake nor rewrite";
    }
}

inline void changeFiles(QDir dir, QStringList dirList)
{
    // move file to target directories, remove another files
    for (const QString &it : dirList)
        QFile::copy(dir.path() + "/" + it + "/license/license.html", dir.path() + "/" + it + "/license.html");

    for (const auto &it : dirList){
        QDir rcDir(dir.path() + "/" + it);
        for (const auto &subdir : rcDir.entryList(QDir::Dirs | QDir::NoDot | QDir::NoDotDot)){
            if (subdir != "svg")
                rmdir(rcDir, subdir);
        }
        QDir svgDir(rcDir);
        if (svgDir.cd("svg")){
            svgDir.setNameFilters({"*.svg"});
            for (const QString &itSvg : svgDir.entryList(QDir::Files | QDir::NoDot | QDir::NoDotDot))
                QFile::copy(svgDir.absoluteFilePath(itSvg), rcDir.absoluteFilePath(itSvg));
        }
        rmdir(rcDir, "svg");
        fixFileName(rcDir);

        // create rc.qrc
        rcDir.setNameFilters({"*.svg"});
        QFile fileRcQrc(rcDir.absoluteFilePath("rc.qrc"));
        if (fileRcQrc.open(QIODevice::Truncate | QIODevice::ReadWrite)){
            QTextStream stream(&fileRcQrc);
            stream << "<RCC>\n    <qresource prefix=\"/icon/" + it + "\">\n";
            for (const QString &svgFile : rcDir.entryList(QDir::Files | QDir::NoDot | QDir::NoDotDot))
                stream << "        " << "<file>" + svgFile + "</file>\n";
            stream << "    </qresource>\n</RCC>";
            fileRcQrc.close();
        }
        else
            qDebug() << "rc.qrc nor rewrite";
    }
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

#ifdef MY_RC_PATH
#define MY_RC_PATH_ MY_RC_PATH
    QDir dir = QString(MY_RC_PATH_);
#else
#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)
    QDir dir(QUOTE(organizationq));
#endif
    dir.cdUp();
    dir.cdUp();
    if (!dir.cd("resources")){
        qDebug() << "Not open resource directory" << dir.path();
        return 1;
    }

    QStringList dirList = dir.entryList(QDir::Dirs | QDir::NoDot | QDir::NoDotDot);
    qDebug() << dir.path() << "\n" << dirList;

    changeFiles(dir, dirList);
    createCmakeFiles(dir, dirList);

    return 0;
}
