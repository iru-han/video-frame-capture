# video-frame-capture

영상을 업로드하면 일정 간격으로 프레임(사진)을 추출해주는 웹앱.
Drogon(C++) 백엔드 + 정적 HTML/JS 프론트엔드로 구성.

## 구조

- `src/` — Drogon 서버 (업로드 / 프레임 추출 / 프레임 삭제 API)
- `frontend/` — 업로드 화면 (index.html, app.js, style.css)
- `data/` — 세션별 업로드 영상 및 추출된 프레임 저장 위치
- `build/` — 빌드 산출물, 실행 파일(`video_frame_capture`)

## 빌드

```bash
cd build
cmake --build .
```

## 실행 / 재시작

서버는 8848 포트를 사용. 코드를 수정한 뒤 반영하려면 기존 프로세스를 종료하고 새 바이너리로 다시 띄워야 함.

```bash
# 1. 기존 서버 종료
pkill video_frame_capture

# 2. (코드 수정했다면) 재빌드
cd ~/video-frame-capture/build
cmake --build .

# 3. 백그라운드로 재시작
nohup ./video_frame_capture > server.log 2>&1 &
disown
```

정상 기동 확인:

```bash
ss -ltnp | grep 8848
```

브라우저에서 접속:

```
http://localhost:8848
```

## 사용법

1. 브라우저에서 접속
2. 영상 파일 선택, 프레임 추출 간격(초) 입력
3. "변환" 클릭 → 업로드 후 자동으로 프레임 추출
4. 예상 사진 수가 150장을 넘으면 경고가 뜨고, 간격을 늘리거나 그대로 진행 선택 가능
5. 추출된 프레임은 갤러리에 표시되며, 각 사진의 ✕ 버튼으로 개별 삭제 가능

## 참고

- 업로드 최대 용량은 2GB로 설정되어 있음 (`src/main.cc`의 `setClientMaxBodySize`)
- 24시간 지난 세션은 서버 시작 시 자동 정리됨 (`SessionManager::cleanupOldSessions`)
