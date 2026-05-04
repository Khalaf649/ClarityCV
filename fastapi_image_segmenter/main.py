import tkinter as tk
from tkinter import filedialog, ttk, messagebox
from PIL import Image, ImageTk
import numpy as np

from config import APP_TITLE, MAX_DISPLAY_SIZE
from utils.image_io import load_image, to_gray, save_image
from algorithms.thresholding import optimal_threshold, otsu_threshold, spectral_threshold, local_threshold
from algorithms.segmentation import kmeans_segmentation, region_growing, agglomerative_segmentation, mean_shift_segmentation


class SegmenterApp(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title(APP_TITLE)
        self.geometry("1180x650")
        self.image = None
        self.gray = None
        self.result = None
        self.seed_point = None
        self._build_ui()

    def _build_ui(self):
        controls = ttk.Frame(self, padding=10)
        controls.pack(side=tk.LEFT, fill=tk.Y)

        ttk.Button(controls, text="Browse image", command=self.open_image).pack(fill=tk.X, pady=4)
        self.method = tk.StringVar(value="otsu")
        methods = ["optimal", "otsu", "spectral", "local", "kmeans", "region_growing", "agglomerative", "mean_shift"]
        ttk.Label(controls, text="Method").pack(anchor="w", pady=(14, 2))
        ttk.Combobox(controls, textvariable=self.method, values=methods, state="readonly").pack(fill=tk.X)

        self.clusters = tk.IntVar(value=4)
        self.levels = tk.IntVar(value=3)
        self.window = tk.IntVar(value=31)
        self.offset = tk.DoubleVar(value=5.0)
        self.threshold = tk.DoubleVar(value=12.0)
        self.bandwidth = tk.DoubleVar(value=0.18)

        self._spin(controls, "Clusters", self.clusters, 2, 10)
        self._spin(controls, "Spectral levels", self.levels, 3, 8)
        self._spin(controls, "Local window", self.window, 3, 101)
        self._entry(controls, "Local offset", self.offset)
        self._entry(controls, "Region threshold", self.threshold)
        self._entry(controls, "Mean-shift bandwidth", self.bandwidth)

        ttk.Button(controls, text="Run", command=self.run_method).pack(fill=tk.X, pady=(18, 4))
        ttk.Button(controls, text="Save result", command=self.save_result).pack(fill=tk.X, pady=4)
        self.status = tk.StringVar(value="Open an image to start. For region growing, click the input image to set a seed.")
        ttk.Label(controls, textvariable=self.status, wraplength=220).pack(fill=tk.X, pady=14)

        display = ttk.Frame(self, padding=10)
        display.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        self.input_label = ttk.Label(display, text="Input", anchor="center")
        self.input_label.grid(row=0, column=0, sticky="nsew")
        self.output_label = ttk.Label(display, text="Output", anchor="center")
        self.output_label.grid(row=0, column=1, sticky="nsew")
        display.columnconfigure(0, weight=1)
        display.columnconfigure(1, weight=1)
        display.rowconfigure(0, weight=1)
        self.input_label.bind("<Button-1>", self.set_seed)

    def _spin(self, parent, label, variable, frm, to):
        ttk.Label(parent, text=label).pack(anchor="w", pady=(10, 2))
        ttk.Spinbox(parent, from_=frm, to=to, textvariable=variable).pack(fill=tk.X)

    def _entry(self, parent, label, variable):
        ttk.Label(parent, text=label).pack(anchor="w", pady=(10, 2))
        ttk.Entry(parent, textvariable=variable).pack(fill=tk.X)

    def _show(self, array, label):
        arr = np.asarray(array)
        if arr.ndim == 2:
            img = Image.fromarray(arr.astype(np.uint8), mode="L")
        else:
            img = Image.fromarray(arr.astype(np.uint8))
        img.thumbnail(MAX_DISPLAY_SIZE)
        photo = ImageTk.PhotoImage(img)
        label.configure(image=photo, text="")
        label.image = photo

    def open_image(self):
        path = filedialog.askopenfilename(filetypes=[("Images", "*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.gif")])
        if not path:
            return
        self.image = load_image(path, mode="RGB")
        self.gray = to_gray(self.image)
        self.result = None
        self.seed_point = None
        self._show(self.image, self.input_label)
        self.output_label.configure(image="", text="Output")
        self.status.set(f"Loaded {path}")

    def set_seed(self, event):
        if self.image is None:
            return
        # Approximate mapping from displayed label click to original image coordinates.
        h, w = self.image.shape[:2]
        dw, dh = min(MAX_DISPLAY_SIZE[0], w), min(MAX_DISPLAY_SIZE[1], h)
        scale = min(MAX_DISPLAY_SIZE[0] / w, MAX_DISPLAY_SIZE[1] / h, 1.0)
        dw, dh = int(w * scale), int(h * scale)
        x0 = max((self.input_label.winfo_width() - dw) // 2, 0)
        y0 = max((self.input_label.winfo_height() - dh) // 2, 0)
        x = int((event.x - x0) / max(scale, 1e-9))
        y = int((event.y - y0) / max(scale, 1e-9))
        self.seed_point = (int(np.clip(x, 0, w - 1)), int(np.clip(y, 0, h - 1)))
        self.status.set(f"Region-growing seed set to {self.seed_point}")

    def run_method(self):
        if self.image is None:
            messagebox.showwarning("No image", "Please open an image first.")
            return
        m = self.method.get()
        try:
            if m == "optimal":
                t, self.result = optimal_threshold(self.gray)
                self.status.set(f"Optimal threshold = {t:.2f}")
            elif m == "otsu":
                t, self.result = otsu_threshold(self.gray)
                self.status.set(f"Otsu threshold = {t}")
            elif m == "spectral":
                ts, self.result = spectral_threshold(self.gray, levels=self.levels.get())
                self.status.set(f"Spectral thresholds = {ts}")
            elif m == "local":
                self.result = local_threshold(self.gray, window_size=self.window.get(), offset=self.offset.get())
                self.status.set("Local thresholding done")
            elif m == "kmeans":
                self.result, _ = kmeans_segmentation(self.image, k=self.clusters.get(), include_xy=True)
                self.status.set("K-means segmentation done")
            elif m == "region_growing":
                self.result = region_growing(self.gray, seed_point=self.seed_point, threshold=self.threshold.get())
                self.status.set("Region growing done")
            elif m == "agglomerative":
                self.result, _ = agglomerative_segmentation(self.image, clusters=self.clusters.get())
                self.status.set("Agglomerative segmentation done")
            elif m == "mean_shift":
                self.result, _ = mean_shift_segmentation(self.image, bandwidth=self.bandwidth.get())
                self.status.set("Mean shift segmentation done")
            self._show(self.result, self.output_label)
        except Exception as exc:
            messagebox.showerror("Error", str(exc))

    def save_result(self):
        if self.result is None:
            messagebox.showwarning("No result", "Run a method first.")
            return
        path = filedialog.asksaveasfilename(defaultextension=".png", filetypes=[("PNG", "*.png"), ("JPEG", "*.jpg")])
        if path:
            save_image(self.result, path)
            self.status.set(f"Saved {path}")


if __name__ == "__main__":
    SegmenterApp().mainloop()
