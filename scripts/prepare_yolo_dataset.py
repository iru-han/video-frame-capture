#!/usr/bin/env python3
"""Collects labeled frames from every session under data/ into a single
YOLO-format dataset (yolo_dataset/) with a train/val split and data.yaml.

Only frames that have a saved label file are included (i.e. skipped/unlabeled
frames are left out). Class ids are remapped so classes are consistent across
sessions, in case sessions registered classes in a different order.

Usage:
    python3 scripts/prepare_yolo_dataset.py [--val-ratio 0.15] [--seed 42]
"""
import argparse
import random
import shutil
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATA_ROOT = PROJECT_ROOT / "data"
OUTPUT_ROOT = PROJECT_ROOT / "yolo_dataset"


def load_classes(session_dir: Path) -> list[str]:
    classes_file = session_dir / "classes.txt"
    if not classes_file.exists():
        return []
    return [line.strip() for line in classes_file.read_text().splitlines() if line.strip()]


def find_image_for_stem(frames_dir: Path, stem: str) -> Path | None:
    for ext in (".jpg", ".jpeg", ".png", ".webp", ".bmp"):
        candidate = frames_dir / (stem + ext)
        if candidate.exists():
            return candidate
    return None


def collect_examples(global_classes: list[str]) -> list[tuple[Path, Path, str]]:
    """Returns list of (image_path, label_path, session_id) for every labeled frame."""
    examples = []
    for session_dir in sorted(DATA_ROOT.iterdir()):
        if not session_dir.is_dir():
            continue
        labels_dir = session_dir / "labels"
        frames_dir = session_dir / "frames"
        if not labels_dir.exists() or not frames_dir.exists():
            continue

        session_classes = load_classes(session_dir)
        if not session_classes:
            continue
        remap = {i: global_classes.index(name) for i, name in enumerate(session_classes)}

        for label_file in sorted(labels_dir.glob("*.txt")):
            image_path = find_image_for_stem(frames_dir, label_file.stem)
            if image_path is None:
                continue
            examples.append((image_path, label_file, session_dir.name, remap))
    return examples


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--val-ratio", type=float, default=0.15)
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()

    if not DATA_ROOT.exists():
        print(f"no data directory found at {DATA_ROOT}")
        return

    global_classes: list[str] = []
    for session_dir in sorted(DATA_ROOT.iterdir()):
        if not session_dir.is_dir():
            continue
        for name in load_classes(session_dir):
            if name not in global_classes:
                global_classes.append(name)

    if not global_classes:
        print("no classes registered in any session yet - label some frames first")
        return

    examples = collect_examples(global_classes)
    if not examples:
        print("no labeled frames found yet - label some frames first")
        return

    random.seed(args.seed)
    random.shuffle(examples)
    val_count = max(1, int(len(examples) * args.val_ratio)) if len(examples) > 1 else 0
    splits = {"val": examples[:val_count], "train": examples[val_count:]}

    if OUTPUT_ROOT.exists():
        shutil.rmtree(OUTPUT_ROOT)

    for split_name, split_examples in splits.items():
        images_dir = OUTPUT_ROOT / "images" / split_name
        labels_dir = OUTPUT_ROOT / "labels" / split_name
        images_dir.mkdir(parents=True, exist_ok=True)
        labels_dir.mkdir(parents=True, exist_ok=True)

        for image_path, label_path, session_id, remap in split_examples:
            unique_name = f"{session_id}_{image_path.stem}"
            shutil.copy2(image_path, images_dir / (unique_name + image_path.suffix))

            lines = []
            for line in label_path.read_text().splitlines():
                if not line.strip():
                    continue
                parts = line.split()
                old_class_id = int(parts[0])
                parts[0] = str(remap[old_class_id])
                lines.append(" ".join(parts))
            (labels_dir / (unique_name + ".txt")).write_text("\n".join(lines) + "\n")

    yaml_lines = [
        f"path: {OUTPUT_ROOT}",
        "train: images/train",
        "val: images/val",
        "names:",
    ]
    for i, name in enumerate(global_classes):
        yaml_lines.append(f"  {i}: {name}")
    (OUTPUT_ROOT / "data.yaml").write_text("\n".join(yaml_lines) + "\n")

    print(f"classes: {global_classes}")
    print(f"train examples: {len(splits['train'])}")
    print(f"val examples: {len(splits['val'])}")
    print(f"dataset written to: {OUTPUT_ROOT}")
    print(f"data.yaml: {OUTPUT_ROOT / 'data.yaml'}")


if __name__ == "__main__":
    main()
