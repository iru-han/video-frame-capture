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
const imageFilesInput = document.getElementById('imageFiles');
const uploadImagesBtn = document.getElementById('uploadImagesBtn');
const hideLabeledToggle = document.getElementById('hideLabeledToggle');

const labelEditor = document.getElementById('labelEditor');
const closeLabelEditorBtn = document.getElementById('closeLabelEditorBtn');
const prevFrameBtn = document.getElementById('prevFrameBtn');
const nextFrameBtn = document.getElementById('nextFrameBtn');
const newClassNameInput = document.getElementById('newClassName');
const addClassBtn = document.getElementById('addClassBtn');
const classList = document.getElementById('classList');
const labelImage = document.getElementById('labelImage');
const labelCanvas = document.getElementById('labelCanvas');
const labelCanvasCtx = labelCanvas.getContext('2d');
const boxList = document.getElementById('boxList');
const saveLabelsBtn = document.getElementById('saveLabelsBtn');
const labelStatus = document.getElementById('labelStatus');

const CLASS_COLORS = ['#dc2626', '#2563eb', '#16a34a', '#d97706', '#7c3aed', '#0891b2', '#db2777', '#65a30d'];
const HANDLE_SIZE = 8;

let state = {
  sessionId: null,
  durationSec: null,
  frames: [],
  labeledSet: new Set(),
  classes: [],
  activeClass: null,
  currentLabelFilename: null,
  boxes: [],
  selectedBoxIndex: null,
};

function clamp(v, min, max) {
  return Math.max(min, Math.min(max, v));
}

function stemOf(filename) {
  const idx = filename.lastIndexOf('.');
  return idx === -1 ? filename : filename.slice(0, idx);
}

function colorForClass(className) {
  const idx = state.classes.indexOf(className);
  return CLASS_COLORS[Math.max(idx, 0) % CLASS_COLORS.length];
}

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

uploadImagesBtn.addEventListener('click', async () => {
  const files = imageFilesInput.files;
  if (!files || files.length === 0) {
    showStatus('사진 파일을 선택해주세요.', true);
    return;
  }

  gallery.innerHTML = '';
  showStatus('업로드 중...');

  try {
    const formData = new FormData();
    for (const file of files) formData.append('images', file);

    const res = await fetch('/api/upload-images', { method: 'POST', body: formData });
    const json = await res.json();
    if (!res.ok) {
      showStatus(json.error || '업로드에 실패했습니다.', true);
      return;
    }

    state.sessionId = json.session_id;
    hideStatus();
    renderGallery(json.frames);
  } catch (err) {
    showStatus('네트워크 오류가 발생했습니다.', true);
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
  state.frames = framePaths.map((p) => p.substring(p.lastIndexOf('/') + 1));

  for (const path of framePaths) {
    const filename = path.substring(path.lastIndexOf('/') + 1);

    const card = document.createElement('div');
    card.className = 'photo-card';
    card.dataset.filename = filename;

    const img = document.createElement('img');
    img.src = path;
    img.addEventListener('click', () => openLabelEditor(filename));
    card.appendChild(img);

    const deleteBtn = document.createElement('button');
    deleteBtn.className = 'delete-btn';
    deleteBtn.textContent = '✕';
    deleteBtn.addEventListener('click', () => deleteFrame(card, filename));
    card.appendChild(deleteBtn);

    gallery.appendChild(card);
  }

  refreshLabeledSet();
}

async function refreshLabeledSet() {
  if (!state.sessionId) return;
  try {
    const res = await fetch(`/api/labels?session_id=${encodeURIComponent(state.sessionId)}`);
    const json = await res.json();
    state.labeledSet = new Set(json.labeled || []);
    applyLabeledBadges();
  } catch (err) {
    // Non-critical: badges just won't reflect saved state until next refresh.
  }
}

function applyLabeledBadges() {
  gallery.querySelectorAll('.photo-card').forEach((card) => {
    const isLabeled = state.labeledSet.has(stemOf(card.dataset.filename));
    card.classList.toggle('labeled', isLabeled);
  });
}

hideLabeledToggle.addEventListener('change', () => {
  gallery.classList.toggle('hide-labeled', hideLabeledToggle.checked);
});

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
      state.frames = state.frames.filter((f) => f !== filename);
    } else {
      deleteBtn.disabled = false;
      showStatus(json.error || '삭제에 실패했습니다.', true);
    }
  } catch (err) {
    deleteBtn.disabled = false;
    showStatus('네트워크 오류가 발생했습니다.', true);
  }
}

function showLabelStatus(text, isError) {
  labelStatus.textContent = text;
  labelStatus.classList.remove('hidden');
  labelStatus.classList.toggle('error', !!isError);
}

function renderClassList() {
  classList.innerHTML = '';
  for (const className of state.classes) {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'class-btn' + (className === state.activeClass ? ' active' : '');
    btn.style.background = colorForClass(className);
    btn.textContent = className;
    btn.addEventListener('click', () => {
      state.activeClass = className;
      renderClassList();
    });
    classList.appendChild(btn);
  }
}

async function loadClasses() {
  const res = await fetch(`/api/classes?session_id=${encodeURIComponent(state.sessionId)}`);
  const json = await res.json();
  state.classes = json.classes || [];
  if (!state.classes.includes(state.activeClass)) {
    state.activeClass = state.classes[0] || null;
  }
  renderClassList();
}

addClassBtn.addEventListener('click', async () => {
  const name = newClassNameInput.value.trim();
  if (!name) return;

  try {
    const res = await fetch('/api/classes', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ session_id: state.sessionId, class: name }),
    });
    const json = await res.json();
    if (!res.ok) {
      showLabelStatus(json.error || '클래스 추가에 실패했습니다.', true);
      return;
    }
    state.classes = json.classes || [];
    state.activeClass = name;
    newClassNameInput.value = '';
    renderClassList();
  } catch (err) {
    showLabelStatus('네트워크 오류가 발생했습니다.', true);
  }
});

function boxRectPx(box) {
  const w = box.width * labelCanvas.width;
  const h = box.height * labelCanvas.height;
  const x1 = box.x_center * labelCanvas.width - w / 2;
  const y1 = box.y_center * labelCanvas.height - h / 2;
  return { x1, y1, x2: x1 + w, y2: y1 + h };
}

function rectToBox(rect, className) {
  const x1 = Math.min(rect.x1, rect.x2);
  const x2 = Math.max(rect.x1, rect.x2);
  const y1 = Math.min(rect.y1, rect.y2);
  const y2 = Math.max(rect.y1, rect.y2);
  return {
    class: className,
    x_center: (x1 + x2) / 2 / labelCanvas.width,
    y_center: (y1 + y2) / 2 / labelCanvas.height,
    width: (x2 - x1) / labelCanvas.width,
    height: (y2 - y1) / labelCanvas.height,
  };
}

function oppositeCorner(rect, corner) {
  if (corner === 'tl') return { x: rect.x2, y: rect.y2 };
  if (corner === 'tr') return { x: rect.x1, y: rect.y2 };
  if (corner === 'bl') return { x: rect.x2, y: rect.y1 };
  return { x: rect.x1, y: rect.y1 };
}

function drawBox(box, selected) {
  const color = colorForClass(box.class);
  const rect = boxRectPx(box);

  labelCanvasCtx.strokeStyle = color;
  labelCanvasCtx.lineWidth = selected ? 3 : 2;
  labelCanvasCtx.strokeRect(rect.x1, rect.y1, rect.x2 - rect.x1, rect.y2 - rect.y1);

  labelCanvasCtx.fillStyle = color;
  labelCanvasCtx.font = '12px sans-serif';
  const textY = rect.y1 > 12 ? rect.y1 - 4 : rect.y1 + 12;
  labelCanvasCtx.fillText(box.class, rect.x1 + 2, textY);

  if (selected) {
    for (const [cx, cy] of [
      [rect.x1, rect.y1],
      [rect.x2, rect.y1],
      [rect.x1, rect.y2],
      [rect.x2, rect.y2],
    ]) {
      labelCanvasCtx.fillRect(cx - 4, cy - 4, 8, 8);
    }
  }
}

function redrawCanvas(previewBox) {
  labelCanvasCtx.clearRect(0, 0, labelCanvas.width, labelCanvas.height);
  state.boxes.forEach((box, idx) => drawBox(box, idx === state.selectedBoxIndex));
  if (previewBox) drawBox(previewBox, false);
}

function renderBoxList() {
  boxList.innerHTML = '';
  state.boxes.forEach((box, idx) => {
    const item = document.createElement('div');
    item.className = 'box-item' + (idx === state.selectedBoxIndex ? ' selected' : '');
    item.addEventListener('click', () => {
      state.selectedBoxIndex = idx;
      redrawCanvas();
      renderBoxList();
    });

    const swatch = document.createElement('span');
    swatch.className = 'swatch';
    swatch.style.background = colorForClass(box.class);
    item.appendChild(swatch);

    const label = document.createElement('span');
    label.textContent = box.class;
    item.appendChild(label);

    const removeBtn = document.createElement('button');
    removeBtn.textContent = '삭제';
    removeBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      state.boxes.splice(idx, 1);
      state.selectedBoxIndex = null;
      redrawCanvas();
      renderBoxList();
    });
    item.appendChild(removeBtn);

    boxList.appendChild(item);
  });
}

function findHandleAt(x, y) {
  for (let i = state.boxes.length - 1; i >= 0; i--) {
    const rect = boxRectPx(state.boxes[i]);
    const corners = {
      tl: { x: rect.x1, y: rect.y1 },
      tr: { x: rect.x2, y: rect.y1 },
      bl: { x: rect.x1, y: rect.y2 },
      br: { x: rect.x2, y: rect.y2 },
    };
    for (const corner of Object.keys(corners)) {
      const pt = corners[corner];
      if (Math.abs(x - pt.x) <= HANDLE_SIZE && Math.abs(y - pt.y) <= HANDLE_SIZE) {
        return { boxIndex: i, corner };
      }
    }
  }
  return null;
}

function findBoxAt(x, y) {
  for (let i = state.boxes.length - 1; i >= 0; i--) {
    const rect = boxRectPx(state.boxes[i]);
    if (x >= rect.x1 && x <= rect.x2 && y >= rect.y1 && y <= rect.y2) return i;
  }
  return null;
}

let interaction = null;

labelCanvas.addEventListener('mousedown', (e) => {
  const rect = labelCanvas.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const y = e.clientY - rect.top;

  const handle = findHandleAt(x, y);
  if (handle) {
    state.selectedBoxIndex = handle.boxIndex;
    interaction = {
      mode: 'resize',
      boxIndex: handle.boxIndex,
      fixed: oppositeCorner(boxRectPx(state.boxes[handle.boxIndex]), handle.corner),
    };
    redrawCanvas();
    renderBoxList();
    return;
  }

  const hitIndex = findBoxAt(x, y);
  if (hitIndex !== null) {
    state.selectedBoxIndex = hitIndex;
    const boxRect = boxRectPx(state.boxes[hitIndex]);
    interaction = {
      mode: 'move',
      boxIndex: hitIndex,
      offsetX: x - boxRect.x1,
      offsetY: y - boxRect.y1,
      width: boxRect.x2 - boxRect.x1,
      height: boxRect.y2 - boxRect.y1,
    };
    redrawCanvas();
    renderBoxList();
    return;
  }

  state.selectedBoxIndex = null;
  if (!state.activeClass) {
    showLabelStatus('먼저 클래스를 선택하거나 추가해주세요.', true);
    redrawCanvas();
    renderBoxList();
    return;
  }
  interaction = { mode: 'draw', startX: x, startY: y };
  redrawCanvas();
});

window.addEventListener('mousemove', (e) => {
  if (!interaction) return;
  const rect = labelCanvas.getBoundingClientRect();
  const x = clamp(e.clientX - rect.left, 0, labelCanvas.width);
  const y = clamp(e.clientY - rect.top, 0, labelCanvas.height);

  if (interaction.mode === 'draw') {
    redrawCanvas(
      rectToBox({ x1: interaction.startX, y1: interaction.startY, x2: x, y2: y }, state.activeClass)
    );
    return;
  }

  if (interaction.mode === 'resize') {
    const box = state.boxes[interaction.boxIndex];
    state.boxes[interaction.boxIndex] = rectToBox(
      { x1: interaction.fixed.x, y1: interaction.fixed.y, x2: x, y2: y },
      box.class
    );
    redrawCanvas();
    return;
  }

  if (interaction.mode === 'move') {
    const box = state.boxes[interaction.boxIndex];
    const newX1 = clamp(x - interaction.offsetX, 0, labelCanvas.width - interaction.width);
    const newY1 = clamp(y - interaction.offsetY, 0, labelCanvas.height - interaction.height);
    state.boxes[interaction.boxIndex] = rectToBox(
      { x1: newX1, y1: newY1, x2: newX1 + interaction.width, y2: newY1 + interaction.height },
      box.class
    );
    redrawCanvas();
  }
});

window.addEventListener('mouseup', (e) => {
  if (!interaction) return;
  const rect = labelCanvas.getBoundingClientRect();
  const x = clamp(e.clientX - rect.left, 0, labelCanvas.width);
  const y = clamp(e.clientY - rect.top, 0, labelCanvas.height);

  if (interaction.mode === 'draw') {
    const w = Math.abs(x - interaction.startX);
    const h = Math.abs(y - interaction.startY);
    if (w >= 5 && h >= 5) {
      state.boxes.push(
        rectToBox({ x1: interaction.startX, y1: interaction.startY, x2: x, y2: y }, state.activeClass)
      );
      renderBoxList();
    }
  }

  interaction = null;
  redrawCanvas();
});

document.addEventListener('keydown', (e) => {
  if (e.key !== 'Delete' && e.key !== 'Backspace') return;
  if (labelEditor.classList.contains('hidden')) return;
  if (document.activeElement && ['INPUT', 'TEXTAREA'].includes(document.activeElement.tagName)) return;
  if (state.selectedBoxIndex === null) return;

  state.boxes.splice(state.selectedBoxIndex, 1);
  state.selectedBoxIndex = null;
  redrawCanvas();
  renderBoxList();
});

function updateNavButtons() {
  const idx = state.frames.indexOf(state.currentLabelFilename);
  prevFrameBtn.disabled = idx <= 0;
  nextFrameBtn.disabled = idx < 0 || idx >= state.frames.length - 1;
}

async function openLabelEditor(filename) {
  state.currentLabelFilename = filename;
  state.boxes = [];
  state.selectedBoxIndex = null;
  interaction = null;
  labelEditor.classList.remove('hidden');
  labelEditor.scrollIntoView({ behavior: 'smooth' });
  updateNavButtons();

  await loadClasses();

  labelImage.onload = async () => {
    labelCanvas.width = labelImage.clientWidth;
    labelCanvas.height = labelImage.clientHeight;

    try {
      const res = await fetch(
        `/api/label?session_id=${encodeURIComponent(state.sessionId)}&filename=${encodeURIComponent(filename)}`
      );
      const json = await res.json();
      state.boxes = json.boxes || [];
    } catch (err) {
      state.boxes = [];
    }
    redrawCanvas();
    renderBoxList();
  };
  labelImage.src = `/data/${state.sessionId}/frames/${filename}`;
}

function navigateFrame(delta) {
  const idx = state.frames.indexOf(state.currentLabelFilename);
  const nextIdx = idx + delta;
  if (nextIdx < 0 || nextIdx >= state.frames.length) {
    showLabelStatus(delta > 0 ? '마지막 사진입니다.' : '첫 번째 사진입니다.', false);
    return;
  }
  openLabelEditor(state.frames[nextIdx]);
}

prevFrameBtn.addEventListener('click', () => navigateFrame(-1));
nextFrameBtn.addEventListener('click', () => navigateFrame(1));

closeLabelEditorBtn.addEventListener('click', () => {
  labelEditor.classList.add('hidden');
});

saveLabelsBtn.addEventListener('click', async () => {
  try {
    const res = await fetch('/api/label', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        session_id: state.sessionId,
        filename: state.currentLabelFilename,
        boxes: state.boxes,
      }),
    });
    const json = await res.json();
    if (!res.ok) {
      showLabelStatus(json.error || '저장에 실패했습니다.', true);
      return;
    }

    state.labeledSet.add(stemOf(state.currentLabelFilename));
    applyLabeledBadges();

    const idx = state.frames.indexOf(state.currentLabelFilename);
    if (idx >= 0 && idx < state.frames.length - 1) {
      showLabelStatus('저장되었습니다. 다음 사진으로 이동합니다.', false);
      openLabelEditor(state.frames[idx + 1]);
    } else {
      showLabelStatus('저장되었습니다. 마지막 사진입니다.', false);
    }
  } catch (err) {
    showLabelStatus('네트워크 오류가 발생했습니다.', true);
  }
});
