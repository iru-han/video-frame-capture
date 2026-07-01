const FRAME_WARNING_THRESHOLD = 150;

const videoFileInput = document.getElementById('videoFile');
const intervalInput = document.getElementById('interval');
const convertBtn = document.getElementById('convertBtn');
const statusText = document.getElementById('statusText');
const warningBanner = document.getElementById('warningBanner');
const warningText = document.getElementById('warningText');
const increaseIntervalBtn = document.getElementById('increaseIntervalBtn');
const proceedBtn = document.getElementById('proceedBtn');
const loading = document.getElementById('loading');
const gallery = document.getElementById('gallery');

let state = { sessionId: null, durationSec: null };

function showStatus(text, isError) {
  statusText.textContent = text;
  statusText.classList.remove('hidden');
  statusText.classList.toggle('error', !!isError);
}

function hideStatus() {
  statusText.classList.add('hidden');
}

function setBusy(busy) {
  convertBtn.disabled = busy;
}

convertBtn.addEventListener('click', async () => {
  const file = videoFileInput.files[0];
  const interval = parseFloat(intervalInput.value);

  if (!file) {
    showStatus('영상 파일을 선택해주세요.', true);
    return;
  }
  if (!interval || interval <= 0) {
    showStatus('간격은 0보다 큰 숫자여야 합니다.', true);
    return;
  }

  warningBanner.classList.add('hidden');
  gallery.innerHTML = '';
  setBusy(true);
  showStatus('업로드 중...');

  try {
    const formData = new FormData();
    formData.append('video', file);

    const uploadRes = await fetch('/api/upload', { method: 'POST', body: formData });
    const uploadJson = await uploadRes.json();
    if (!uploadRes.ok) {
      showStatus(uploadJson.error || '업로드에 실패했습니다.', true);
      setBusy(false);
      return;
    }

    state.sessionId = uploadJson.session_id;
    state.durationSec = uploadJson.duration_sec;
    hideStatus();

    checkEstimateAndProceed();
  } catch (err) {
    showStatus('네트워크 오류가 발생했습니다.', true);
    setBusy(false);
  }
});

function checkEstimateAndProceed() {
  const interval = parseFloat(intervalInput.value);
  const estimatedFrames = Math.ceil(state.durationSec / interval);

  if (estimatedFrames > FRAME_WARNING_THRESHOLD) {
    warningText.textContent =
      `예상 사진 수: ${estimatedFrames}장 — 시간이 걸릴 수 있습니다. 간격을 늘리시겠습니까?`;
    warningBanner.classList.remove('hidden');
    setBusy(false);
    return;
  }

  runExtraction(interval);
}

increaseIntervalBtn.addEventListener('click', () => {
  const suggested = Math.ceil(state.durationSec / FRAME_WARNING_THRESHOLD);
  intervalInput.value = suggested;
  const estimatedFrames = Math.ceil(state.durationSec / suggested);
  warningText.textContent = `예상 사진 수: ${estimatedFrames}장 (간격 ${suggested}초로 조정됨)`;
});

proceedBtn.addEventListener('click', () => {
  warningBanner.classList.add('hidden');
  const interval = parseFloat(intervalInput.value);
  runExtraction(interval);
});

async function runExtraction(interval) {
  setBusy(true);
  loading.classList.remove('hidden');
  hideStatus();

  try {
    const res = await fetch('/api/extract', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ session_id: state.sessionId, interval }),
    });
    const json = await res.json();
    loading.classList.add('hidden');
    setBusy(false);

    if (!res.ok) {
      showStatus(json.error || '프레임 추출에 실패했습니다.', true);
      return;
    }

    renderGallery(json.frames);
  } catch (err) {
    loading.classList.add('hidden');
    setBusy(false);
    showStatus('네트워크 오류가 발생했습니다.', true);
  }
}

function renderGallery(framePaths) {
  gallery.innerHTML = '';
  for (const path of framePaths) {
    const filename = path.substring(path.lastIndexOf('/') + 1);

    const card = document.createElement('div');
    card.className = 'photo-card';
    card.dataset.filename = filename;

    const img = document.createElement('img');
    img.src = path;
    card.appendChild(img);

    const deleteBtn = document.createElement('button');
    deleteBtn.className = 'delete-btn';
    deleteBtn.textContent = '✕';
    deleteBtn.addEventListener('click', () => deleteFrame(card, filename));
    card.appendChild(deleteBtn);

    gallery.appendChild(card);
  }
}

async function deleteFrame(card, filename) {
  const deleteBtn = card.querySelector('.delete-btn');
  deleteBtn.disabled = true;

  try {
    const res = await fetch('/api/frame', {
      method: 'DELETE',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ session_id: state.sessionId, filename }),
    });
    const json = await res.json();

    if (res.ok && json.ok) {
      card.remove();
    } else {
      deleteBtn.disabled = false;
      showStatus(json.error || '삭제에 실패했습니다.', true);
    }
  } catch (err) {
    deleteBtn.disabled = false;
    showStatus('네트워크 오류가 발생했습니다.', true);
  }
}
