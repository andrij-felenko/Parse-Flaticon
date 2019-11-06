#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>

#include <iostream>
#include <cstdarg>

inline void fixFileName(QDir dir)
{
    for (auto it : dir.entryList(QDir::Files)){
        if (it.right(4) != ".svg")
            continue;
        QString newName = it;
        newName.remove(newName.length() - 4, 4);
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
            if (dir.entryList(QDir::Files).contains(newName + posStr + ".svg"))
                newI++;
            else {
                newName += posStr;
                break;
            }
        }
        QFile::rename(dir.absoluteFilePath(it), dir.absoluteFilePath(newName.replace('-', '_').replace(' ', '_')).toLower() + ".svg");
    }
}

inline void rmdir(QDir dir, QString subDir)
{
    QDir subdir(dir);
    if (! subdir.cd(subDir))
        return;
    qDebug() << subdir.path() << subDir;
    for (auto i : subdir.entryList(QDir::Files))
        subdir.remove(i);
    for (auto i : subdir.entryList(QDir::Dirs | QDir::NoDot | QDir::NoDotDot))
        rmdir(subdir, i);

    dir.rmdir(subDir);
}

inline void createQbsFiles(QDir dir, QStringList dirList)
{
    // update resource.qbs
    QFile file(dir.absoluteFilePath("resource.qbs"));
    if (file.open(QIODevice::Truncate | QIODevice::ReadWrite)){
        QTextStream stream(&file);
        stream << "import qbs\n\nProject {\n    references: [\n";
        for (QString it : dirList)
            stream << "        " << "\"" << it << "/" << it << ".qbs\",\n";
        stream << "    ]\n}";
        file.close();
    }
    else
        qDebug() << "resource.qbs nor rewrite";

    for (auto it : dirList){
        QDir rcDir(dir.path() + "/" + it);
        fixFileName(rcDir);

        // create it.qbs
        QFile itFile(rcDir.absoluteFilePath(it + ".qbs"));
        if (itFile.open(QIODevice::Truncate | QIODevice::ReadWrite)){
            QTextStream stream(&itFile);
            stream << "import qbs\n"
                   << "import \"../../templateQbs/templateLibRes.qbs\" as LibStaticRes\n\n"
                   << "LibStaticRes {\n    resourceName: \"" + it + "\"\n}";
            itFile.close();
        }
        else
            qDebug() << "rc.qbs nor rewrite";
    }
}

inline void createCmakeFiles(QDir dir, QStringList dirList)
{
    // update resource.qbs
    QFile file(dir.absoluteFilePath("CMakeLists.txt"));
    if (file.open(QIODevice::Truncate | QIODevice::ReadWrite)){
        QTextStream stream(&file);
        for (QString it : dirList)
            stream << "add_subdirectory(" << it << ")\n";
        file.close();
    }
    else
        qDebug() << "cmake not rewrite";

    for (auto it : dirList){
        QDir rcDir(dir.path() + "/" + it);
        fixFileName(rcDir);

        // create it.qbs
        QFile itFile(rcDir.absoluteFilePath("CMakeLists.txt"));
        if (itFile.open(QIODevice::Truncate | QIODevice::ReadWrite)){
            QTextStream stream(&itFile);
            stream << "cmake_minimum_required(VERSION 3.14)\n"
                      "project(rc_" << it << " VERSION 1.0.0)\n"
                      "\n"
                   << "set (CMAKE_AUTORCC ON)\n"
                      "find_package(Qt5Core)\n"
                      "add_library(rc_" << it << " rc.qrc)\n"
                      "target_link_libraries(rc_" << it << " Qt5::Core)";
            itFile.close();
        }
        else
            qDebug() << "cmake nor rewrite";
    }
}

inline void changeFiles(QDir dir, QStringList dirList)
{
    // move file to target directories, remove another files
    for (QString it : dirList)
        QFile::copy(dir.path() + "/" + it + "/license/license.html", dir.path() + "/" + it + "/license.html");

    for (auto it : dirList){
        QDir rcDir(dir.path() + "/" + it);
        for (auto subdir : rcDir.entryList(QDir::Dirs | QDir::NoDot | QDir::NoDotDot)){
            if (subdir != "svg")
                rmdir(rcDir, subdir);
        }
        QDir svgDir(rcDir);
        if (svgDir.cd("svg")){
            svgDir.setNameFilters({"*.svg"});
            for (QString itSvg : svgDir.entryList(QDir::Files | QDir::NoDot | QDir::NoDotDot))
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
            for (QString svgFile : rcDir.entryList(QDir::Files | QDir::NoDot | QDir::NoDotDot))
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
    createQbsFiles(dir, dirList);
    createCmakeFiles(dir, dirList);

    return 0;
}
