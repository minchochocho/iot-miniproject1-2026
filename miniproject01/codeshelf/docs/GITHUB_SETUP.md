# GitHub에 CodeShelf만 올리기

현재 이 폴더는 상위 저장소 `iot-miniproject1-2026` 안에 포함되어 있습니다.  
포트폴리오용 **단독 저장소**로 올리려면 아래 순서를 따르세요.

## 1. 스크린샷 준비 (권장)

`docs/screenshots/` 에 다음 파일을 추가한 뒤 `README.md` 의 이미지 주석을 해제하세요.

- `main.png` — 메인 화면
- `scan.png` — 스캔 진행 상태바 (선택)
- `demo.gif` — 15~30초 사용 영상 (선택, 임팩트 큼)

## 2. GitHub에서 새 저장소 생성

1. GitHub → **New repository**
2. Name: `CodeShelf` (또는 `codeshelf`)
3. Public, README **추가 안 함**
4. Create repository

## 3. 이 폴더만 새 저장소로 push

PowerShell 예시 (경로는 본인 환경에 맞게 수정):

```powershell
cd C:\SourceBank\iot-miniproject1-2026\miniproject01\codeshelf

git init
git add .
git commit -m "docs: add portfolio README and CodeShelf source"
git branch -M main
git remote add origin https://github.com/minchochocho/CodeShelf.git
git push -u origin main
```

## 4. GitHub 저장소 꾸미기

| 항목 | 추천 |
|------|------|
| About | Description: `Qt/C++ local code indexer with MySQL` |
| Topics | `qt`, `cpp`, `mysql`, `desktop-app`, `iot`, `code-search` |
| Pin | 프로필에서 이 저장소 Pin |
| Releases | 빌드된 `codeshelf.exe`는 Release에 첨부 (선택) |

## 5. README Pin용 한 줄 (About)

```
C++/Qt6 + MySQL local source indexer — background scan, DB pagination, syntax preview
```

## 6. 주의

- `DatabaseManager.h` 에 DB 비밀번호가 있으면, 공개 저장소 push 전에 **환경 변수/설정 파일로 분리**하는 것을 권장합니다. → [docs/SECURITY.md](SECURITY.md)
- `x64/`, `.vs/` 는 `.gitignore`에 포함되어 있어 커밋되지 않아야 합니다.

## 7. 이력서에 넣을 때

README 하단 **Portfolio blurb** 문단을 그대로 복사해 사용하면 됩니다.
