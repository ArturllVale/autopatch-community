<script setup lang="ts">
import { ref, computed } from 'vue'
import { useI18n } from 'vue-i18n'
import { useProjectStore } from '../../stores/project'

const { t } = useI18n()
const projectStore = useProjectStore()

// Current active code tab
const activeCodeTab = ref<'html' | 'css' | 'js'>('html')

// Size inputs
const windowWidth = computed({
  get: () => projectStore.project.config.windowWidth,
  set: (val) => projectStore.updateConfig({ windowWidth: val })
})

const windowHeight = computed({
  get: () => projectStore.project.config.windowHeight,
  set: (val) => projectStore.updateConfig({ windowHeight: val })
})

// Current content based on active tab
const currentContent = computed({
  get: () => {
    switch (activeCodeTab.value) {
      case 'html':
        return projectStore.project.config.htmlContent || defaultHtml
      case 'css':
        return projectStore.project.config.cssContent || defaultCss
      case 'js':
        return projectStore.project.config.jsContent || defaultJs
      default:
        return ''
    }
  },
  set: (value: string) => {
    switch (activeCodeTab.value) {
      case 'html':
        projectStore.updateConfig({ htmlContent: value })
        break
      case 'css':
        projectStore.updateConfig({ cssContent: value })
        break
      case 'js':
        projectStore.updateConfig({ jsContent: value })
        break
    }
  }
})

// Preview HTML content
const previewHtml = computed(() => {
  const html = projectStore.project.config.htmlContent || defaultHtml
  const css = projectStore.project.config.cssContent || defaultCss
  const js = projectStore.project.config.jsContent || defaultJs
  
  // Inject CSS and JS into HTML
  let fullHtml = html
  
  // Replace link to style.css with inline styles
  if (fullHtml.includes('href="style.css"')) {
    fullHtml = fullHtml.replace(
      /<link[^>]*href="style\.css"[^>]*>/gi,
      `<style>${css}</style>`
    )
  } else if (!fullHtml.includes('<style>')) {
    fullHtml = fullHtml.replace('</head>', `<style>${css}</style></head>`)
  }
  
  // Replace script src with inline script
  if (fullHtml.includes('src="script.js"')) {
    fullHtml = fullHtml.replace(
      /<script[^>]*src="script\.js"[^>]*><\/script>/gi,
      `<script>${js}<\/script>`
    )
  } else if (!fullHtml.includes(js) && js) {
    fullHtml = fullHtml.replace('</body>', `<script>${js}<\/script></body>`)
  }
  
  return fullHtml
})

function loadBasicTemplate() {
  projectStore.updateConfig({
    htmlContent: defaultHtml,
    cssContent: defaultCss,
    jsContent: defaultJs
  })
}

function loadAdvancedTemplate() {
  projectStore.updateConfig({
    htmlContent: advancedHtml,
    cssContent: advancedCss,
    jsContent: advancedJs
  })
}

function refreshPreview() {
  // Force re-render of iframe by toggling a key
  const iframe = document.querySelector('.preview-iframe') as HTMLIFrameElement
  if (iframe) {
    iframe.srcdoc = previewHtml.value
  }
}

const defaultHtml = `<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>AutoPatcher</title>
</head>
<body>
<div class="container">
    <div class="header drag-region">
        <h1>Meu Servidor</h1>
        <div class="window-controls">
            <button onclick="patcher.minimize()">−</button>
            <button onclick="patcher.close()">×</button>
        </div>
    </div>

    <div class="content">
        <div class="news">
            <h2>Novidades</h2>
            <p>Bem-vindo ao nosso servidor!</p>
        </div>

        <div class="status-area">
            <div id="status">Verificando atualizações ... </div>
            <div class="progress-container">
                <div id="progress-bar"></div>
            </div>
            <div id="progress-text">0%</div>
        </div>
    </div>

    <div class="footer">
        <button id="btn-start" onclick="patcher.startGame()" disabled>
            INICIAR JOGO
        </button>
    </div>
</div>
</body>
</html>`

const defaultCss = `* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Segoe UI', Tahoma, sans-serif;
    background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
    color: #fff;
    height: 100vh;
    overflow: hidden;
}

.container {
    display: flex;
    flex-direction: column;
    height: 100%;
}

.header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 15px 20px;
    background: rgba(0,0,0,0.3);
}

.header h1 {
    font-size: 20px;
    font-weight: 500;
}

.window-controls {
    display: flex;
    gap: 8px;
}

.window-controls button {
    width: 30px;
    height: 30px;
    border: none;
    background: rgba(255,255,255,0.1);
    color: #fff;
    border-radius: 4px;
    cursor: pointer;
    font-size: 16px;
}

.window-controls button:hover {
    background: rgba(255,255,255,0.2);
}

.content {
    flex: 1;
    padding: 20px;
    overflow: auto;
}

.news {
    background: rgba(255,255,255,0.05);
    border-radius: 8px;
    padding: 20px;
    margin-bottom: 20px;
}

.news h2 {
    font-size: 16px;
    margin-bottom: 10px;
    color: #00cc6a;
}

.status-area {
    background: rgba(0,0,0,0.2);
    border-radius: 8px;
    padding: 15px;
}

#status {
    font-size: 13px;
    color: #aaa;
    margin-bottom: 10px;
}

.progress-container {
    height: 8px;
    background: rgba(255,255,255,0.1);
    border-radius: 4px;
    overflow: hidden;
    margin-bottom: 8px;
}

#progress-bar {
    height: 100%;
    width: 0%;
    background: linear-gradient(90deg, #00cc6a, #00ff88);
    transition: width 0.3s;
}

#progress-text {
    font-size: 12px;
    color: #00cc6a;
    text-align: right;
}

.footer {
    padding: 20px;
    background: rgba(0,0,0,0.3);
    text-align: center;
}

#btn-start {
    padding: 15px 60px;
    font-size: 16px;
    font-weight: 600;
    border: none;
    border-radius: 6px;
    background: #00cc6a;
    color: #fff;
    cursor: pointer;
    transition: all 0.2s;
}

#btn-start:hover:not(:disabled) {
    background: #00ff88;
    transform: translateY(-2px);
}

#btn-start:disabled {
    background: #333;
    color: #666;
    cursor: not-allowed;
}`

const defaultJs = `// Callbacks do AutoPatcher
window.onPatcherStatus = function(status) {
    document.getElementById('status').textContent = status;
};

window.onPatcherProgress = function(percent) {
    document.getElementById('progress-bar').style.width = percent + '%';
    document.getElementById('progress-text').textContent = percent + '%';
};

window.onPatcherComplete = function() {
    document.getElementById('btn-start').disabled = false;
    document.getElementById('status').textContent = 'Pronto para jogar!';
};

window.onPatcherError = function(message) {
    document.getElementById('status').textContent = 'Erro: ' + message;
    document.getElementById('status').style.color = '#ff4444';
};`

const advancedHtml = `<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <title>AutoPatcher - Advanced</title>
</head>
<body>
<div class="patcher">
    <header class="drag-region">
        <div class="server-logo">
            <span>MEU SERVIDOR RO</span>
        </div>
        <div class="window-buttons">
            <button class="minimize" onclick="patcher.minimize()">−</button>
            <button class="close" onclick="patcher.close()">×</button>
        </div>
    </header>

    <main>
        <section class="news-panel">
            <div class="news-header">
                <h2>📢 Novidades</h2>
            </div>
            <div class="news-content">
                <div class="news-item">
                    <h3>Evento de Natal!</h3>
                    <p>Participe do nosso evento especial!</p>
                </div>
            </div>
        </section>

        <section class="status-panel">
            <div class="server-status">
                <span class="status-dot online"></span>
                <span>Servidor Online</span>
            </div>
            
            <div class="update-status">
                <div id="status">Verificando atualizações...</div>
                <div class="progress-wrapper">
                    <div id="progress-bar"></div>
                </div>
                <div id="progress-text">0%</div>
            </div>
        </section>
    </main>

    <footer>
        <button id="btn-start" onclick="patcher.startGame()" disabled>
            INICIAR JOGO
        </button>
    </footer>
</div>
</body>
</html>`

const advancedCss = `* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Segoe UI', sans-serif;
    background: #0d0d14;
    color: #fff;
    height: 100vh;
}

.patcher {
    display: flex;
    flex-direction: column;
    height: 100%;
    background: linear-gradient(180deg, #1a1a2e 0%, #0d0d14 100%);
}

header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 20px;
    background: rgba(0,0,0,0.5);
}

.server-logo span {
    font-size: 16px;
    font-weight: 600;
}

.window-buttons {
    display: flex;
    gap: 4px;
}

.window-buttons button {
    width: 36px;
    height: 36px;
    border: none;
    background: transparent;
    color: #888;
    border-radius: 6px;
    cursor: pointer;
    font-size: 18px;
}

.window-buttons button:hover {
    background: rgba(255,255,255,0.1);
    color: #fff;
}

.window-buttons .close:hover {
    background: #e81123;
}

main {
    flex: 1;
    display: grid;
    grid-template-columns: 1fr 280px;
    gap: 20px;
    padding: 20px;
}

.news-panel {
    background: rgba(255,255,255,0.03);
    border-radius: 12px;
    border: 1px solid rgba(255,255,255,0.05);
}

.news-header {
    padding: 16px 20px;
    border-bottom: 1px solid rgba(255,255,255,0.05);
}

.news-header h2 {
    font-size: 14px;
}

.news-content {
    padding: 20px;
}

.news-item {
    padding: 16px;
    background: rgba(0,0,0,0.3);
    border-radius: 8px;
}

.news-item h3 {
    font-size: 14px;
    color: #00cc6a;
    margin-bottom: 8px;
}

.news-item p {
    font-size: 13px;
    color: #888;
}

.status-panel {
    display: flex;
    flex-direction: column;
    gap: 16px;
}

.server-status {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 16px;
    background: rgba(255,255,255,0.03);
    border-radius: 12px;
}

.status-dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: #00cc6a;
    box-shadow: 0 0 10px #00cc6a;
}

.update-status {
    flex: 1;
    padding: 20px;
    background: rgba(255,255,255,0.03);
    border-radius: 12px;
}

#status {
    font-size: 13px;
    color: #888;
    margin-bottom: 16px;
}

.progress-wrapper {
    height: 6px;
    background: rgba(255,255,255,0.1);
    border-radius: 3px;
    overflow: hidden;
    margin-bottom: 12px;
}

#progress-bar {
    height: 100%;
    width: 0%;
    background: linear-gradient(90deg, #00cc6a, #00ff88);
    transition: width 0.3s;
}

#progress-text {
    font-size: 12px;
    color: #00cc6a;
    text-align: right;
}

footer {
    padding: 16px 20px;
    background: rgba(0,0,0,0.5);
    text-align: center;
}

#btn-start {
    padding: 14px 48px;
    font-size: 14px;
    font-weight: 600;
    border: none;
    border-radius: 8px;
    background: #00cc6a;
    color: white;
    cursor: pointer;
}

#btn-start:hover:not(:disabled) {
    background: #00ff88;
}

#btn-start:disabled {
    background: #333;
    color: #666;
    cursor: not-allowed;
}`

const advancedJs = `// Callbacks do AutoPatcher
window.onPatcherStatus = function(status) {
    document.getElementById('status').textContent = status;
};

window.onPatcherProgress = function(percent) {
    document.getElementById('progress-bar').style.width = percent + '%';
    document.getElementById('progress-text').textContent = percent + '%';
};

window.onPatcherComplete = function() {
    document.getElementById('btn-start').disabled = false;
    document.getElementById('status').textContent = 'Arquivos atualizados!';
};

window.onPatcherError = function(message) {
    document.getElementById('status').textContent = 'Erro: ' + message;
    document.getElementById('status').style.color = '#ff4444';
};`
</script>

<template>
  <div class="html-editor-container">
    <!-- Header -->
    <div class="editor-header">
      <h2 class="editor-title">Editor de Interface - Modo HTML/CSS/JS</h2>
    </div>

    <div class="editor-content">
      <!-- Left side - Code Editor -->
      <div class="code-panel">
        <!-- Code Tabs -->
        <div class="code-header">
          <div class="code-tabs">
            <button
              class="code-tab"
              :class="{ active: activeCodeTab === 'html' }"
              @click="activeCodeTab = 'html'"
            >
              <span class="tab-badge html">HTML</span>
              HTML
            </button>
            <button
              class="code-tab"
              :class="{ active: activeCodeTab === 'css' }"
              @click="activeCodeTab = 'css'"
            >
              <span class="tab-badge css">CSS</span>
              CSS
            </button>
            <button
              class="code-tab"
              :class="{ active: activeCodeTab === 'js' }"
              @click="activeCodeTab = 'js'"
            >
              <span class="tab-badge js">JS</span>
              JavaScript
            </button>
          </div>

          <div class="size-controls">
            <span>{{ t('codeEditor.size') }}:</span>
            <input type="number" v-model="windowWidth" min="400" max="1920" />
            <span>x</span>
            <input type="number" v-model="windowHeight" min="300" max="1080" />
          </div>

          <div class="template-buttons">
            <button class="template-btn" @click="loadBasicTemplate">
              <svg viewBox="0 0 24 24" fill="currentColor"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zM6 20V4h7v5h5v11H6z"/></svg>
              {{ t('codeEditor.basicTemplate') }}
            </button>
            <button class="template-btn" @click="loadAdvancedTemplate">
              <svg viewBox="0 0 24 24" fill="currentColor"><path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zM6 20V4h7v5h5v11H6z"/></svg>
              {{ t('codeEditor.advancedTemplate') }}
            </button>
          </div>
        </div>

        <!-- Code Textarea -->
        <div class="code-wrapper">
          <div class="code-file-name">
            {{ activeCodeTab === 'html' ? 'index.html' : activeCodeTab === 'css' ? 'style.css' : 'script.js' }}
          </div>
          <textarea
            class="code-textarea"
            v-model="currentContent"
            spellcheck="false"
            @input="refreshPreview"
          ></textarea>
        </div>
      </div>

      <!-- Right side - Preview -->
      <div class="preview-panel">
        <div class="preview-header">
          <span>{{ t('codeEditor.preview') }}</span>
          <button class="refresh-btn" @click="refreshPreview" :title="t('codeEditor.refreshPreview')">
            <svg viewBox="0 0 24 24" fill="currentColor"><path d="M17.65 6.35C16.2 4.9 14.21 4 12 4c-4.42 0-7.99 3.58-7.99 8s3.57 8 7.99 8c3.73 0 6.84-2.55 7.73-6h-2.08c-.82 2.33-3.04 4-5.65 4-3.31 0-6-2.69-6-6s2.69-6 6-6c1.66 0 3.14.69 4.22 1.78L13 11h7V4l-2.35 2.35z"/></svg>
          </button>
        </div>
        <div class="preview-container">
          <iframe
            class="preview-iframe"
            :srcdoc="previewHtml"
            sandbox="allow-scripts"
            :style="{
              width: windowWidth + 'px',
              height: windowHeight + 'px',
              transform: `scale(${Math.min(1, 450 / windowWidth, 400 / windowHeight)})`
            }"
          ></iframe>
        </div>
        <div class="preview-note">
          <svg viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-8h-2V7h2v2z"/></svg>
          <span>APIs do patcher (patcher.startGame()) funcionarão apenas no executável final.</span>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.html-editor-container {
  flex: 1;
  display: flex;
  flex-direction: column;
  background-color: #1e1e1e;
  overflow: hidden;
}

.editor-header {
  padding: 16px 24px;
  border-bottom: 1px solid #3e3e42;
}

.editor-title {
  font-size: 16px;
  font-weight: 500;
  color: #cccccc;
  margin: 0;
}

.editor-content {
  flex: 1;
  display: flex;
  overflow: hidden;
  padding: 16px;
  gap: 16px;
}

/* Code Panel */
.code-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  background: #252526;
  border-radius: 8px;
  border: 1px solid #3e3e42;
  overflow: hidden;
}

.code-header {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 12px 16px;
  border-bottom: 1px solid #3e3e42;
  flex-wrap: wrap;
}

.code-tabs {
  display: flex;
  gap: 4px;
}

.code-tab {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  background: transparent;
  border: none;
  border-radius: 4px;
  color: #9d9d9d;
  font-size: 13px;
  cursor: pointer;
  transition: all 0.15s;
}

.code-tab:hover {
  background: #2d2d30;
  color: #cccccc;
}

.code-tab.active {
  background: #0e639c;
  color: #ffffff;
}

.tab-badge {
  font-size: 9px;
  font-weight: 700;
  padding: 2px 5px;
  border-radius: 3px;
}

.tab-badge.html {
  background: #e44d26;
  color: white;
}

.tab-badge.css {
  background: #264de4;
  color: white;
}

.tab-badge.js {
  background: #f7df1e;
  color: black;
}

.size-controls {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-left: auto;
  color: #9d9d9d;
  font-size: 12px;
}

.size-controls input {
  width: 60px;
  padding: 6px 8px;
  background: #1e1e1e;
  border: 1px solid #3e3e42;
  border-radius: 4px;
  color: #cccccc;
  font-size: 12px;
  text-align: center;
}

.size-controls input:focus {
  outline: none;
  border-color: #0078d4;
}

.template-buttons {
  display: flex;
  gap: 8px;
}

.template-btn {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 12px;
  background: #2d2d30;
  border: 1px solid #3e3e42;
  border-radius: 4px;
  color: #cccccc;
  font-size: 12px;
  cursor: pointer;
  transition: all 0.15s;
}

.template-btn:hover {
  background: #3e3e42;
  border-color: #4e4e52;
}

.template-btn svg {
  width: 14px;
  height: 14px;
}

.code-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.code-file-name {
  padding: 8px 16px;
  background: #1e1e1e;
  border-bottom: 1px solid #3e3e42;
  font-size: 12px;
  color: #9d9d9d;
}

.code-textarea {
  flex: 1;
  width: 100%;
  background: #1e1e1e;
  color: #d4d4d4;
  border: none;
  padding: 16px;
  font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
  font-size: 13px;
  line-height: 1.6;
  resize: none;
  outline: none;
  tab-size: 2;
}

.code-textarea::selection {
  background-color: #264f78;
}

/* Preview Panel */
.preview-panel {
  width: 480px;
  display: flex;
  flex-direction: column;
  background: #252526;
  border-radius: 8px;
  border: 1px solid #3e3e42;
  overflow: hidden;
  flex-shrink: 0;
}

.preview-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  border-bottom: 1px solid #3e3e42;
  font-size: 13px;
  color: #cccccc;
}

.refresh-btn {
  width: 28px;
  height: 28px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: transparent;
  border: 1px solid #3e3e42;
  border-radius: 4px;
  color: #9d9d9d;
  cursor: pointer;
  transition: all 0.15s;
}

.refresh-btn:hover {
  background: #3e3e42;
  color: #cccccc;
}

.refresh-btn svg {
  width: 16px;
  height: 16px;
}

.preview-container {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #0d0d0d;
  overflow: hidden;
  padding: 16px;
}

.preview-iframe {
  border: none;
  border-radius: 4px;
  background: #1e1e1e;
  transform-origin: center center;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.5);
}

.preview-note {
  display: flex;
  align-items: flex-start;
  gap: 8px;
  padding: 12px 16px;
  background: rgba(255, 193, 7, 0.1);
  border-top: 1px solid #3e3e42;
  font-size: 11px;
  color: #9d9d9d;
  line-height: 1.4;
}

.preview-note svg {
  width: 14px;
  height: 14px;
  flex-shrink: 0;
  color: #ffc107;
  margin-top: 1px;
}
</style>
