"""
Upload Tab - Handle uploading local files to server
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import subprocess
import threading
import re
import os

class UploadTab(ttk.Frame):
    def __init__(self, parent):
        super().__init__(parent, padding="20")
        
        self.is_uploading = False
        self.current_process = None
        self.selected_filepath = None
        
        self.columnconfigure(0, weight=1)
        
        self.create_widgets()
        
    def create_widgets(self):
        """Create all widgets for upload tab"""
        row = 0
        
        file_frame = ttk.LabelFrame(self, text="📤 Select File to Upload", padding="15")
        file_frame.grid(row=row, column=0, sticky=(tk.W, tk.E), pady=(0, 15))
        file_frame.columnconfigure(1, weight=1)
        
        ttk.Label(file_frame, 
                 text="File:", 
                 font=('Helvetica', 10, 'bold')).grid(row=0, column=0, sticky=tk.W, padx=(0, 10))
        
        self.file_entry = ttk.Entry(file_frame, font=('Consolas', 10))
        self.file_entry.grid(row=0, column=1, sticky=(tk.W, tk.E), padx=(0, 10))
        
        ttk.Button(
            file_frame,
            text="📁 Browse...",
            command=self.browse_file,
            width=14
        ).grid(row=0, column=2)
        
        row += 1
        
        info_frame = ttk.LabelFrame(self, text="ℹ️  File Information", padding="15")
        info_frame.grid(row=row, column=0, sticky=(tk.W, tk.E), pady=(0, 15))
        
        self.info_text = tk.Text(
            info_frame,
            height=4,
            font=('Courier', 9),
            bg='#ecf0f1',
            fg='#2c3e50',
            state='disabled',
            wrap='word'
        )
        self.info_text.pack(fill=tk.BOTH, expand=True)
        
        row += 1
        
        options_frame = ttk.LabelFrame(self, text="⚙️  Upload Options", padding="15")
        options_frame.grid(row=row, column=0, sticky=(tk.W, tk.E), pady=(0, 15))
        
        self.dedup_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(
            options_frame,
            text="✅ Enable deduplication (recommended)",
            variable=self.dedup_var
        ).grid(row=0, column=0, sticky=tk.W, pady=5)
        
        ttk.Label(
            options_frame,
            text="Deduplication will split the file into chunks and store only unique chunks,\n"
                 "saving storage space when similar files are uploaded.",
            font=('Helvetica', 9),
            foreground='#7f8c8d'
        ).grid(row=1, column=0, sticky=tk.W, pady=(0, 5))
        
        row += 1
        
        progress_frame = ttk.LabelFrame(self, text="📊 Upload Progress", padding="15")
        progress_frame.grid(row=row, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 15))
        progress_frame.columnconfigure(0, weight=1)
        progress_frame.rowconfigure(3, weight=1)
        
        self.progress_var = tk.DoubleVar()
        self.progress_bar = ttk.Progressbar(
            progress_frame,
            style='Green.Horizontal.TProgressbar',
            mode='determinate',
            variable=self.progress_var,
            length=400
        )
        self.progress_bar.grid(row=0, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        
        self.status_label = ttk.Label(
            progress_frame,
            text="Ready to upload",
            font=('Helvetica', 10)
        )
        self.status_label.grid(row=1, column=0, sticky=tk.W, pady=(0, 5))
        
        self.stats_label = ttk.Label(
            progress_frame,
            text="",
            font=('Courier', 9),
            foreground='#27ae60'
        )
        self.stats_label.grid(row=2, column=0, sticky=tk.W, pady=(0, 10))
        
        console_label = ttk.Label(progress_frame, 
                                 text="Console Output:",
                                 font=('Helvetica', 10, 'bold'))
        console_label.grid(row=3, column=0, sticky=(tk.W, tk.N), pady=(5, 5))
        text_frame = ttk.Frame(progress_frame)
        text_frame.grid(row=4, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        text_frame.columnconfigure(0, weight=1)
        text_frame.rowconfigure(0, weight=1)
        
        self.console_text = tk.Text(
            text_frame,
            height=8,
            width=80,
            font=('Consolas', 9),
            bg='#1e272e',
            fg='#00d2d3',
            insertbackground='#00d2d3',
            selectbackground='#3498db',
            selectforeground='white',
            state='disabled',
            wrap='word',
            borderwidth=2,
            relief='flat',
            padx=8,
            pady=8
        )
        self.console_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        scrollbar = ttk.Scrollbar(text_frame, orient='vertical', command=self.console_text.yview)
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        self.console_text['yscrollcommand'] = scrollbar.set
        
        row += 1
        
        button_frame = ttk.Frame(self)
        button_frame.grid(row=row, column=0, sticky=(tk.W, tk.E))
        
        self.upload_btn = ttk.Button(
            button_frame,
            text="▶  Upload to Server",
            command=self.start_upload,
            style='Primary.TButton',
            width=22
        )
        self.upload_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        self.cancel_btn = ttk.Button(
            button_frame,
            text="⏹  Cancel",
            command=self.cancel_upload,
            state='disabled',
            width=15
        )
        self.cancel_btn.pack(side=tk.LEFT, padx=(0, 10))
        
        ttk.Button(
            button_frame,
            text="🗑️  Clear Console",
            command=self.clear_console,
            width=17
        ).pack(side=tk.LEFT)
    
    def browse_file(self):
        """Open file browser dialog"""
        filename = filedialog.askopenfilename(
            title="Select file to upload",
            initialdir=os.path.expanduser("~"),
            filetypes=[
                ("All files", "*.*"),
                ("Video files", "*.mp4 *.mkv *.avi"),
                ("Documents", "*.pdf *.doc *.docx"),
                ("Images", "*.jpg *.png *.gif"),
            ]
        )
        
        if filename:
            self.selected_filepath = filename
            self.file_entry.delete(0, tk.END)
            self.file_entry.insert(0, filename)
            self.update_file_info(filename)
    
    def update_file_info(self, filepath):
        """Update file information display"""
        if not os.path.exists(filepath):
            return
        
        file_size = os.path.getsize(filepath)
        file_name = os.path.basename(filepath)
        
        if file_size < 1024:
            size_str = f"{file_size} bytes"
        elif file_size < 1024 * 1024:
            size_str = f"{file_size / 1024:.2f} KB"
        elif file_size < 1024 * 1024 * 1024:
            size_str = f"{file_size / (1024 * 1024):.2f} MB"
        else:
            size_str = f"{file_size / (1024 * 1024 * 1024):.2f} GB"
        
        chunk_size = 256 * 1024
        chunks = (file_size + chunk_size - 1) // chunk_size
        
        info = f"Filename: {file_name}\n"
        info += f"Size: {size_str}\n"
        info += f"Estimated chunks: {chunks}\n"
        info += f"Server identifier: local://{file_name}"
        
        self.info_text.config(state='normal')
        self.info_text.delete(1.0, tk.END)
        self.info_text.insert(1.0, info)
        self.info_text.config(state='disabled')
    
    def append_console(self, text):
        """Append text to console output"""
        self.console_text.config(state='normal')
        self.console_text.insert(tk.END, text)
        self.console_text.see(tk.END)
        self.console_text.config(state='disabled')
    
    def clear_console(self):
        """Clear console output"""
        self.console_text.config(state='normal')
        self.console_text.delete(1.0, tk.END)
        self.console_text.config(state='disabled')
    
    def start_upload(self):
        """Start the upload process"""
        filepath = self.file_entry.get().strip()
        
        if not filepath:
            messagebox.showwarning("Input Required", "Please select a file to upload")
            return
        
        if not os.path.exists(filepath):
            messagebox.showerror("File Not Found", f"File does not exist:\n{filepath}")
            return
        
        if self.is_uploading:
            messagebox.showwarning("Upload in Progress", "An upload is already in progress")
            return
        
        self.selected_filepath = filepath
        
        self.clear_console()
        self.progress_var.set(0)
        self.status_label.config(text="Starting upload...")
        self.stats_label.config(text="")
        
        self.upload_btn.config(state='disabled')
        self.cancel_btn.config(state='normal')
        self.is_uploading = True
        
        thread = threading.Thread(target=self._run_upload, args=(filepath,), daemon=True)
        thread.start()
    
    def _run_upload(self, filepath):
        """Run upload in background thread"""
        try:
            import sys
            
            # Handle both PyInstaller executable and normal Python script
            if getattr(sys, 'frozen', False):
                # Running as PyInstaller executable
                application_path = sys._MEIPASS
                client_path = os.path.join(application_path, 'client_app')
            else:
                # Running as normal Python script
                script_dir = os.path.dirname(os.path.abspath(__file__))
                project_root = os.path.abspath(os.path.join(script_dir, '..'))
                client_path = os.path.join(project_root, 'client_app')
            
            if not os.path.exists(client_path):
                self.append_console(f"ERROR: Client not found at {client_path}\n")
                self.append_console("Please build the project first: ./build_all.sh\n")
                self._upload_complete(False)
                return
            
            # Load server settings
            settings = self.load_settings()
            server_ip = settings.get('server_ip', '127.0.0.1')
            server_port = settings.get('server_port', 8080)
            
            self.append_console(f"Uploading file: {filepath}\n")
            self.append_console(f"Target server: {server_ip}:{server_port}\n")
            self.append_console("-" * 60 + "\n\n")
            
            # Set environment variables for client
            env = os.environ.copy()
            env['SERVER_IP'] = server_ip
            env['SERVER_PORT'] = str(server_port)
            
            process = subprocess.Popen(
                [client_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                cwd=project_root,
                env=env
            )
            
            self.current_process = process
            upload_success = False
            
            try:
                process.stdin.write("2\n")
                process.stdin.write(f"{filepath}\n")
                process.stdin.flush()
                process.stdin.close()
            except:
                pass
            
            for line in process.stdout:
                if not self.is_uploading:
                    break
                
                self.append_console(line)
                
                progress_match = re.search(r'(\d+)%', line)
                if progress_match:
                    percent = int(progress_match.group(1))
                    self.progress_var.set(percent)
                    self.status_label.config(text=f"Uploading: {percent}%")
                
                if "Duplicates found:" in line:
                    self.stats_label.config(text=line.strip())
                elif "Space saved:" in line:
                    current = self.stats_label.cget("text")
                    self.stats_label.config(text=current + " | " + line.strip())
                
                if "Uploading to server" in line:
                    self.status_label.config(text="Uploading chunks...")
                elif "SUCCESS" in line:
                    self.status_label.config(text="Upload complete!")
                    self.progress_var.set(100)
                    upload_success = True
                elif "File already exists on server" in line or "Rejecting duplicate" in line:
                    # Server rejected duplicate - this is still a success (deduplication working)
                    self.status_label.config(text="File already on server (deduplicated)")
                    self.progress_var.set(100)
                    upload_success = True
            
            process.wait()
            # Consider it success if we saw SUCCESS message OR file already exists (dedup)
            success = upload_success or (process.returncode == 0)
            
            self._upload_complete(success)
            
        except Exception as e:
            self.append_console(f"\nERROR: {str(e)}\n")
            self._upload_complete(False)
    
    def _upload_complete(self, success):
        """Called when upload completes"""
        self.is_uploading = False
        self.current_process = None
        
        self.after(0, lambda: self.upload_btn.config(state='normal'))
        self.after(0, lambda: self.cancel_btn.config(state='disabled'))
        
        if success:
            self.after(0, lambda: self.status_label.config(text="✓ Upload completed successfully!"))
            self.after(0, lambda: self.progress_var.set(100))
            
            if self.selected_filepath:
                filename = os.path.basename(self.selected_filepath)
                download_url = f"local://{filename}"
                
                try:
                    self.clipboard_clear()
                    self.clipboard_append(download_url)
                    self.update()
                    
                    msg = f"File uploaded successfully!\n\n"
                    msg += f"To download this file, use:\n{download_url}\n\n"
                    msg += f"✓ URL copied to clipboard!"
                    
                    self.after(0, lambda: messagebox.showinfo("Success", msg))
                except:
                    msg = f"File uploaded successfully!\n\n"
                    msg += f"To download this file, use:\n{download_url}"
                    self.after(0, lambda: messagebox.showinfo("Success", msg))
            else:
                self.after(0, lambda: messagebox.showinfo("Success", "File uploaded to server successfully!"))
        else:
            self.after(0, lambda: self.status_label.config(text="✗ Upload failed"))
            self.after(0, lambda: messagebox.showerror("Error", "Upload failed. Check console for details."))
    
    def load_settings(self):
        """Load settings from settings.json"""
        import json
        settings_path = os.path.join(os.path.dirname(__file__), 'settings.json')
        try:
            if os.path.exists(settings_path):
                with open(settings_path, 'r') as f:
                    return json.load(f)
        except:
            pass
        return {'server_ip': '127.0.0.1', 'server_port': 8080}
    
    def cancel_upload(self):
        """Cancel ongoing upload"""
        if self.current_process:
            self.current_process.terminate()
            self.append_console("\n\n[CANCELLED BY USER]\n")
        
        self.is_uploading = False
        self.upload_btn.config(state='normal')
        self.cancel_btn.config(state='disabled')
        self.status_label.config(text="Upload cancelled")
