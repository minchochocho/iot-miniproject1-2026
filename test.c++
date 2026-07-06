void CodeShelf::onSelectRootFolder() {
    QString dirPath = QFileDialog::getExistingDirectory(this, "루트 폴더 선택", QDir::homePath());
    
    if (!dirPath.isEmpty()) {
        currentRootPath = dirPath;
        categoryTree->clear(); // 기존 목록 삭제

        // 최상위 루트 아이템 생성
        QFileInfo rootInfo(dirPath);
        QTreeWidgetItem* rootItem = new QTreeWidgetItem(categoryTree);
        rootItem->setText(0, rootInfo.fileName());
        rootItem->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
        rootItem->setExpanded(true); // 처음엔 펼쳐두기

        scanDirRecursive(dirPath, rootItem);
    }
}

void CodeShelf::scanDirRecursive(const QString& path, QTreeWidgetItem* parentItem) {
    QDir directory(path);
    
    // 1. 파일 및 폴더 리스트 가져오기 (숨김파일 제외, 이름순 정렬)
    QFileInfoList entries = directory.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo& info : entries) {
        QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
        item->setText(0, info.fileName());

        if (info.isDir()) {
            // [폴더인 경우] 아이콘 설정 후 재귀 호출
            item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
            scanDirRecursive(info.absoluteFilePath(), item);
        } else {
            // [파일인 경우] 확장자 필터링 (C++, SQL, Python 등)
            QString suffix = info.suffix().toLower();
            if (suffix == "cpp" || suffix == "h" || suffix == "sql" || suffix == "py") {
                item->setIcon(0, style()->standardIcon(QStyle::SP_FileIcon));
                item->setData(0, Qt::UserRole, info.absoluteFilePath()); // 실제 경로 저장
            } else {
                delete item; // 원치 않는 파일은 트리에서 제거
            }
        }
    }
}