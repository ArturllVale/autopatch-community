import { app, shell, BrowserWindow, ipcMain, dialog } from 'electron'
import { join } from 'path'
import { electronApp, optimizer, is } from '@electron-toolkit/utils'
import { readFile, writeFile, copyFile, mkdir } from 'fs/promises'
import { spawn } from 'child_process'
import icon from '../../resources/icon.png?asset'

// Global error handler
process.on('uncaughtException', (error) => {
  console.error('[MAIN ERROR] Uncaught exception:', error)
})

process.on('unhandledRejection', (reason) => {
  console.error('[MAIN ERROR] Unhandled rejection:', reason)
})

console.log('[MAIN] Starting application...')

function createWindow(): void {
  // Create the browser window.
  const mainWindow = new BrowserWindow({
    width: 1280,
    height: 800,
    minWidth: 1024,
    minHeight: 600,
    show: false,
    autoHideMenuBar: true,
    backgroundColor: '#1e1e1e',
    ...(process.platform === 'linux' ? { icon } : {}),
    webPreferences: {
      preload: join(__dirname, '../preload/index.js'),
      sandbox: false,
      contextIsolation: true,
      webSecurity: false // Allow loading local file:// resources
    }
  })

  mainWindow.on('ready-to-show', () => {
    mainWindow.show()
  })

  mainWindow.webContents.setWindowOpenHandler((details) => {
    shell.openExternal(details.url)
    return { action: 'deny' }
  })

  // HMR for renderer base on electron-vite cli.
  // Load the remote URL for development or the local html file for production.
  if (is.dev && process.env['ELECTRON_RENDERER_URL']) {
    mainWindow.loadURL(process.env['ELECTRON_RENDERER_URL'])
  } else {
    mainWindow.loadFile(join(__dirname, '../renderer/index.html'))
  }
}

// Setup IPC handlers
function setupIpcHandlers(): void {
  console.log('[MAIN] Setting up IPC handlers...')
  
  // Test handler
  ipcMain.handle('test:ping', async () => {
    console.log('[MAIN] test:ping received!')
    return 'pong'
  })
  
  // Dialog: Open file
  ipcMain.handle('dialog:open-file', async (_event, options) => {
    const result = await dialog.showOpenDialog({
      title: options?.title || 'Abrir Arquivo',
      filters: options?.filters || [{ name: 'Todos os Arquivos', extensions: ['*'] }],
      properties: ['openFile']
    })
    return result.canceled ? null : result.filePaths[0]
  })

  // Dialog: Save file
  ipcMain.handle('dialog:save-file', async (_event, options) => {
    const result = await dialog.showSaveDialog({
      title: options?.title || 'Salvar Arquivo',
      filters: options?.filters || [{ name: 'Todos os Arquivos', extensions: ['*'] }],
      defaultPath: options?.defaultPath
    })
    return result.canceled ? null : result.filePath
  })

  // Dialog: Show message box
  ipcMain.handle('dialog:show-message', async (_event, options) => {
    return await dialog.showMessageBox({
      type: options?.type || 'info',
      title: options?.title || 'Mensagem',
      message: options?.message || '',
      detail: options?.detail
    })
  })

  // File: Read
  ipcMain.handle('file:read', async (_event, path: string) => {
    return await readFile(path, 'utf-8')
  })

  // File: Read Binary (returns base64)
  ipcMain.handle('file:read-binary', async (_event, path: string) => {
    console.log('[MAIN] file:read-binary called with path:', path)
    try {
      const buffer = await readFile(path)
      console.log('[MAIN] Read file, buffer size:', buffer.length)
      const base64 = buffer.toString('base64')
      console.log('[MAIN] Converted to base64, length:', base64.length)
      return base64
    } catch (e) {
      console.error('[MAIN] Error reading file:', e)
      throw e
    }
  })

  // File: Write
  ipcMain.handle('file:write', async (_event, { path, content }) => {
    await writeFile(path, content, 'utf-8')
    return true
  })

  console.log('[MAIN] Registering build:generate-exe handler...')
  
  // Build: Generate EXE
  ipcMain.handle('build:generate-exe', async (_event, options) => {
    console.log('[BUILD] ========== HANDLER CALLED ==========')
    console.log('[BUILD] Options received:', JSON.stringify(options).substring(0, 100))
    
    const fs = require('fs')
    const path = require('path')
    
    try {
      const { config, outputPath, backgroundImagePath, iconPath } = options

      console.log('[BUILD] Starting patcher generation...')
      console.log('[BUILD] Output path:', outputPath)

      // Handle video background - save original path for later copying
      let videoSourcePath = ''
      let videoFileName = ''
      if (config.videoBackground?.enabled && config.videoBackground?.path) {
        videoSourcePath = config.videoBackground.path
        if (fs.existsSync(videoSourcePath)) {
          // Get just the filename
          videoFileName = path.basename(videoSourcePath)
          // Update config to use just the filename (video will be in same folder as EXE)
          config.videoBackground.videoFile = videoFileName
          // Remove the full path from config (we don't want to store it)
          delete config.videoBackground.path
          console.log('[BUILD] Video source:', videoSourcePath)
          console.log('[BUILD] Video file:', videoFileName)
        } else {
          console.log('[BUILD] Video file not found:', videoSourcePath)
        }
      }

      // 1. Find the AutoPatcher.exe template
      // In dev: relative to project root in cpp/build/bin/Release
      // In production: bundled with app resources
      let templatePath: string
      let embedderPath: string
      
      if (is.dev) {
        // Development paths
        // __dirname in dev is: electron-builder/out/main
        // We need to go up 2 levels to get to electron-builder, then up 1 more to autoPatch Community
        const electronBuilderRoot = join(__dirname, '../..')  // electron-builder/
        const autoPatchRoot = join(electronBuilderRoot, '..')  // autoPatch Community/
        
        templatePath = join(autoPatchRoot, 'cpp/build/bin/Release/AutoPatcher.exe')
        embedderPath = join(electronBuilderRoot, 'native/build/bin/embedder.exe')
        
        console.log('[BUILD] Dev mode - __dirname:', __dirname)
        console.log('[BUILD] Dev mode - electronBuilderRoot:', electronBuilderRoot)
        console.log('[BUILD] Dev mode - autoPatchRoot:', autoPatchRoot)
        console.log('[BUILD] Template path:', templatePath)
        console.log('[BUILD] Embedder path:', embedderPath)
      } else {
        templatePath = join(process.resourcesPath, 'AutoPatcher.exe')
        embedderPath = join(process.resourcesPath, 'embedder.exe')
      }

      // Check if template exists
      if (!fs.existsSync(templatePath)) {
        console.error('[BUILD] Template not found at:', templatePath)
        return { 
          success: false, 
          error: `Template AutoPatcher.exe não encontrado em: ${templatePath}\n\nCertifique-se de que o projeto C++ foi compilado.`
        }
      }

      // 2. Write config to temp file
      const tempConfigPath = join(app.getPath('temp'), 'autopatch_config.json')
      await writeFile(tempConfigPath, JSON.stringify(config, null, 2), 'utf-8')
      console.log('[BUILD] Config written to:', tempConfigPath)

      // 3. Check if embedder exists
      if (!fs.existsSync(embedderPath)) {
        console.log('[BUILD] Embedder not found, using copy method')
        
        // Fallback: Just copy the template and write a separate config file
        await copyFile(templatePath, outputPath)
        
        // Write config next to the exe
        const configOutputPath = outputPath.replace(/\.exe$/i, '_config.json')
        await writeFile(configOutputPath, JSON.stringify(config, null, 2), 'utf-8')
        
        return { 
          success: true, 
          message: `Patcher copiado para: ${outputPath}\nConfiguração salva em: ${configOutputPath}\n\nNota: O embedder não foi encontrado, então a configuração não foi embutida no EXE.`
        }
      }

      // 4. Run embedder to create the final EXE
      const args = [
        '--config', tempConfigPath,
        '--template', templatePath,
        '--output', outputPath
      ]

      if (backgroundImagePath && fs.existsSync(backgroundImagePath)) {
        args.push('--background', backgroundImagePath)
        console.log('[BUILD] Background image:', backgroundImagePath)
      }

      if (iconPath && fs.existsSync(iconPath)) {
        args.push('--icon', iconPath)
        console.log('[BUILD] Icon:', iconPath)
      }

      console.log('[BUILD] Running embedder with args:', args)

      return new Promise((resolve) => {
        const child = spawn(embedderPath, args)
        let stdout = ''
        let stderr = ''

        child.stdout.on('data', (data) => {
          const text = data.toString()
          stdout += text
          console.log('[BUILD] stdout:', text)
        })

        child.stderr.on('data', (data) => {
          const text = data.toString()
          stderr += text
          console.log('[BUILD] stderr:', text)
        })

        child.on('close', async (code) => {
          console.log('[BUILD] Process exited with code:', code)
          if (code === 0) {
            // Copy video file if enabled (to resources subfolder)
            if (videoSourcePath && videoFileName) {
              try {
                const outputDir = path.dirname(outputPath)
                const resourcesDir = path.join(outputDir, 'resources')
                
                // Create resources folder if it doesn't exist
                await mkdir(resourcesDir, { recursive: true })
                
                const videoDestPath = path.join(resourcesDir, videoFileName)
                await copyFile(videoSourcePath, videoDestPath)
                console.log('[BUILD] Video copied to:', videoDestPath)
              } catch (err: any) {
                console.error('[BUILD] Failed to copy video:', err)
              }
            }
            resolve({ success: true, message: stdout || 'Patcher gerado com sucesso!' })
          } else {
            resolve({ success: false, error: stderr || stdout || `Processo terminou com código ${code}` })
          }
        })

        child.on('error', (err) => {
          console.error('[BUILD] Process error:', err)
          resolve({ success: false, error: err.message })
        })
      })
    } catch (error: any) {
      console.error('[BUILD] Exception:', error)
      return { success: false, error: error.message }
    }
  })
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.whenReady().then(() => {
  // Set app user model id for windows
  electronApp.setAppUserModelId('com.autopatch.builder')

  // Default open or close DevTools by F12 in development
  // and ignore CommandOrControl + R in production.
  // see https://github.com/alex8088/electron-toolkit/tree/master/packages/utils
  app.on('browser-window-created', (_, window) => {
    optimizer.watchWindowShortcuts(window)
  })

  // Setup IPC handlers
  setupIpcHandlers()

  // IPC test
  ipcMain.on('ping', () => console.log('pong'))

  createWindow()

  app.on('activate', () => {
    // On macOS it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

// Quit when all windows are closed, except on macOS. There, it's common
// for applications and their menu bar to stay active until the user quits
// explicitly with Cmd + Q.
app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.
