#ifndef __Tester_H__
#define __Tester_H__

// Qt headers
#include <QtGui/QWidget>
#include <QtGui/QPushButton>
#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QDialog>

// blocks
#include <FileSearch/FileSearch.h>
#include <FileSearchUi/FileSearchUi.h>


class Tester : public QWidget
{
    Q_OBJECT

    public:
        Tester();
        ~Tester();

/*
    private slots:
        void slot_showManager();
        
        void slot_createFile();
        void slot_copyFile();
        void slot_removeFile();
        void slot_renameFile();
        void slot_moveFile();
        
        void slot_createFolder();
        void slot_copyFolder();
        void slot_removeFolder();
        void slot_renameFolder();
        void slot_moveFolder();
        
        void slot_copyObjectsToFolder();
        void slot_moveObjectsToFolder();
        void slot_removeObjects();
        
        void slot_createSymbolicLink();

        void slot_markObjectsForCopy();
        void slot_markObjectsForCut();
        void slot_pasteObjects();

    private:
        FileSearch*   _fileSearch;
        FileSearchUi* _fileSearchUi;
        */
};

#endif
