// Tipos de elementos UI
export type ElementType = 'button' | 'label' | 'progress' | 'status' | 'percentage' | 'box' | 'image' | 'webview'

// Estado visual de um botão/elemento interativo
export interface ElementState {
  imagePath?: string         // Imagem do estado
  backgroundColor?: string   // Cor de fundo (se não usar imagem)
  fontColor?: string         // Cor do texto neste estado
  borderColor?: string       // Cor da borda
  opacity?: number           // Opacidade (0-100)
  offsetX?: number           // Deslocamento X ao pressionar
  offsetY?: number           // Deslocamento Y ao pressionar
}

// Efeitos visuais aplicáveis a elementos
export interface ElementEffects {
  shadow?: {
    enabled: boolean
    color: string
    blur: number
    offsetX: number
    offsetY: number
  }
  glow?: {
    enabled: boolean
    color: string
    intensity: number
  }
  opacity?: number           // Opacidade geral (0-100)
  borderRadius?: number      // Arredondamento das bordas
  rotation?: number          // Rotação em graus
}

export interface UIElement {
  id: string
  type: ElementType
  name?: string              // Nome amigável para identificação
  x: number
  y: number
  width: number
  height: number
  text: string
  action?: string            // Para botões: start_game, check_files, settings, close, minimize, exit
  
  // Fonte
  fontName?: string
  fontSize?: number
  fontColor?: string
  fontBold?: boolean
  fontItalic?: boolean
  textAlign?: 'left' | 'center' | 'right'
  textVerticalAlign?: 'top' | 'middle' | 'bottom'
  
  // Aparência base
  backgroundColor?: string
  backgroundImage?: string   // Caminho para imagem de fundo
  borderColor?: string
  borderWidth?: number
  
  // Estados (para botões e elementos interativos)
  states?: {
    normal?: ElementState
    hover?: ElementState
    pressed?: ElementState
    disabled?: ElementState
  }
  
  // Efeitos visuais
  effects?: ElementEffects
  
  // Box específico
  boxStyle?: {
    fillColor: string
    fillOpacity: number      // 0-100
    borderColor: string
    borderWidth: number
    borderRadius: number
  }
  
  // WebView específico (para exibir conteúdo web externo)
  webviewConfig?: {
    url: string              // URL da página a ser exibida
    borderRadius: number     // Arredondamento das bordas
    borderColor: string      // Cor da borda
    borderWidth: number      // Largura da borda
    backgroundColor: string  // Cor de fundo enquanto carrega
  }
  
  // Controle de visibilidade e camadas
  visible?: boolean
  locked?: boolean           // Se true, não pode ser movido/editado
  zIndex?: number            // Ordem de renderização
  
  isSelected?: boolean
}

export interface ProgressBarConfig {
  x: number
  y: number
  width: number
  height: number
  backgroundColor: string
  fillColor: string
  borderColor: string
}

export interface ProjectConfig {
  // Informações do servidor
  serverName: string
  patchListUrl: string
  newsUrl?: string

  // Cliente
  clientExe: string
  clientArgs?: string
  grfFiles: string[]

  // Ícone do patcher (arquivo .ico)
  iconPath?: string

  // UI
  windowWidth: number
  windowHeight: number
  windowBorderRadius: number  // Arredondamento das bordas da janela (0 = sem arredondamento)
  uiMode: 'image' | 'html'

  // Elementos (modo imagem)
  elements: UIElement[]
  progressBar: ProgressBarConfig
  backgroundImagePath?: string

  // HTML mode
  htmlContent?: string
  cssContent?: string
  jsContent?: string
}

export interface Project {
  name: string
  path?: string
  config: ProjectConfig
  isDirty: boolean
}

// IPC Types
export interface DialogFilter {
  name: string
  extensions: string[]
}

export interface OpenFileOptions {
  title?: string
  filters?: DialogFilter[]
  defaultPath?: string
}

export interface SaveFileOptions {
  title?: string
  filters?: DialogFilter[]
  defaultPath?: string
}

export interface BuildOptions {
  config: ProjectConfig
  templatePath: string
  outputPath: string
  backgroundImagePath?: string
  iconPath?: string
}

export interface BuildProgress {
  percent: number
  message: string
}

// Default values
export function createDefaultProject(): Project {
  return {
    name: 'Novo Projeto',
    isDirty: false,
    config: {
      serverName: 'Meu Servidor RO',
      patchListUrl: 'http://exemplo.com/patchlist.txt',
      clientExe: 'ragexe.exe',
      grfFiles: ['data.grf'],
      windowWidth: 800,
      windowHeight: 600,
      windowBorderRadius: 0,
      uiMode: 'image',
      elements: [],
      progressBar: {
        x: 50,
        y: 550,
        width: 700,
        height: 20,
        backgroundColor: '#333333',
        fillColor: '#00ff00',
        borderColor: '#666666'
      },
      // Default HTML mode content
      htmlContent: getDefaultHtml(),
      cssContent: getDefaultCss(),
      jsContent: getDefaultJs()
    }
  }
}

// Default HTML template
function getDefaultHtml(): string {
  return `<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <title>Patcher</title>
  <link rel="stylesheet" href="style.css">
</head>
<body>
  <div class="patcher-container">
    <div class="header">
      <h1 id="server-name">Meu Servidor RO</h1>
      <div class="window-controls">
        <button id="btn-minimize" class="btn-control">−</button>
        <button id="btn-close" class="btn-control btn-close">×</button>
      </div>
    </div>
    
    <div class="content">
      <div class="news-panel">
        <h2>Notícias</h2>
        <div id="news-content">Carregando notícias...</div>
      </div>
    </div>
    
    <div class="footer">
      <div class="status-area">
        <span id="status-text">Pronto para iniciar</span>
        <span id="progress-percent">0%</span>
      </div>
      <div class="progress-container">
        <div id="progress-bar" class="progress-bar"></div>
      </div>
      <button id="btn-start" class="btn-start">INICIAR JOGO</button>
    </div>
  </div>
  <script src="script.js"></script>
</body>
</html>`
}

function getDefaultCss(): string {
  return `* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: 'Segoe UI', sans-serif;
  background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%);
  color: #fff;
  overflow: hidden;
  user-select: none;
}

.patcher-container {
  width: 100vw;
  height: 100vh;
  display: flex;
  flex-direction: column;
}

.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px 20px;
  background: rgba(0, 0, 0, 0.3);
  -webkit-app-region: drag;
}

.header h1 {
  font-size: 18px;
  font-weight: 600;
}

.window-controls {
  display: flex;
  gap: 5px;
  -webkit-app-region: no-drag;
}

.btn-control {
  width: 30px;
  height: 30px;
  border: none;
  background: rgba(255, 255, 255, 0.1);
  color: #fff;
  font-size: 16px;
  cursor: pointer;
  border-radius: 4px;
  transition: background 0.2s;
}

.btn-control:hover {
  background: rgba(255, 255, 255, 0.2);
}

.btn-close:hover {
  background: #e81123;
}

.content {
  flex: 1;
  padding: 20px;
  overflow: auto;
}

.news-panel {
  background: rgba(0, 0, 0, 0.2);
  border-radius: 8px;
  padding: 15px;
  height: 100%;
}

.news-panel h2 {
  font-size: 14px;
  margin-bottom: 10px;
  color: #4ec9b0;
}

#news-content {
  font-size: 12px;
  color: #ccc;
  line-height: 1.6;
}

.footer {
  padding: 15px 20px;
  background: rgba(0, 0, 0, 0.3);
}

.status-area {
  display: flex;
  justify-content: space-between;
  margin-bottom: 8px;
  font-size: 12px;
}

#status-text {
  color: #4ec9b0;
}

#progress-percent {
  color: #ffd700;
}

.progress-container {
  height: 20px;
  background: rgba(0, 0, 0, 0.3);
  border-radius: 10px;
  overflow: hidden;
  margin-bottom: 15px;
}

.progress-bar {
  height: 100%;
  width: 0%;
  background: linear-gradient(90deg, #4ec9b0, #00d4aa);
  border-radius: 10px;
  transition: width 0.3s;
}

.btn-start {
  width: 100%;
  padding: 12px;
  border: none;
  background: linear-gradient(90deg, #4ec9b0, #00d4aa);
  color: #1a1a2e;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  border-radius: 6px;
  transition: transform 0.2s, box-shadow 0.2s;
}

.btn-start:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 15px rgba(78, 201, 176, 0.4);
}

.btn-start:disabled {
  opacity: 0.6;
  cursor: not-allowed;
  transform: none;
}`
}

function getDefaultJs(): string {
  return `// Patcher JavaScript
// IDs usados pelo patcher C++:
// - btn-start: Botão iniciar jogo
// - btn-close: Botão fechar
// - btn-minimize: Botão minimizar
// - progress-bar: Barra de progresso (CSS width)
// - status-text: Texto de status
// - progress-percent: Porcentagem

document.addEventListener('DOMContentLoaded', function() {
  console.log('Patcher loaded');
  
  // Simula carregamento de notícias
  setTimeout(function() {
    document.getElementById('news-content').innerHTML = 
      '<p>Bem-vindo ao servidor!</p>' +
      '<p>Use o botão abaixo para iniciar o jogo.</p>';
  }, 500);
});`
}

export function createDefaultButton(id: string): UIElement {
  return {
    id,
    type: 'button',
    name: 'Botão',
    x: 100,
    y: 100,
    width: 120,
    height: 40,
    text: 'Botão',
    action: 'start_game',
    fontName: 'Segoe UI',
    fontSize: 14,
    fontColor: '#ffffff',
    fontBold: true,
    textAlign: 'center',
    textVerticalAlign: 'middle',
    backgroundColor: '#0078d4',
    borderColor: '#005a9e',
    borderWidth: 1,
    states: {
      normal: {
        backgroundColor: '#0078d4',
        fontColor: '#ffffff'
      },
      hover: {
        backgroundColor: '#1e90ff',
        fontColor: '#ffffff'
      },
      pressed: {
        backgroundColor: '#004578',
        fontColor: '#cccccc',
        offsetY: 2
      },
      disabled: {
        backgroundColor: '#666666',
        fontColor: '#999999',
        opacity: 60
      }
    },
    effects: {
      opacity: 100,
      borderRadius: 4
    },
    visible: true,
    locked: false,
    zIndex: 10
  }
}

export function createDefaultLabel(id: string): UIElement {
  return {
    id,
    type: 'label',
    name: 'Label',
    x: 100,
    y: 100,
    width: 200,
    height: 24,
    text: 'Label',
    fontName: 'Segoe UI',
    fontSize: 12,
    fontColor: '#ffffff',
    fontBold: false,
    textAlign: 'left',
    textVerticalAlign: 'middle',
    effects: {
      opacity: 100
    },
    visible: true,
    locked: false,
    zIndex: 5
  }
}

export function createDefaultStatusLabel(id: string): UIElement {
  return {
    id,
    type: 'status',
    name: 'Status',
    x: 50,
    y: 520,
    width: 400,
    height: 24,
    text: 'Verificando atualizações...',
    fontName: 'Segoe UI',
    fontSize: 12,
    fontColor: '#00ff80',
    fontBold: false,
    textAlign: 'left',
    textVerticalAlign: 'middle',
    effects: {
      opacity: 100,
      shadow: {
        enabled: true,
        color: '#000000',
        blur: 2,
        offsetX: 1,
        offsetY: 1
      }
    },
    visible: true,
    locked: false,
    zIndex: 20
  }
}

export function createDefaultPercentageLabel(id: string): UIElement {
  return {
    id,
    type: 'percentage',
    name: 'Porcentagem',
    x: 720,
    y: 550,
    width: 60,
    height: 24,
    text: '100%',
    fontName: 'Segoe UI',
    fontSize: 12,
    fontColor: '#ffcc00',
    fontBold: true,
    textAlign: 'right',
    textVerticalAlign: 'middle',
    effects: {
      opacity: 100
    },
    visible: true,
    locked: false,
    zIndex: 20
  }
}

export function createDefaultBox(id: string): UIElement {
  return {
    id,
    type: 'box',
    name: 'Box',
    x: 100,
    y: 100,
    width: 200,
    height: 150,
    text: '',
    boxStyle: {
      fillColor: '#000000',
      fillOpacity: 50,
      borderColor: '#ffffff',
      borderWidth: 1,
      borderRadius: 8
    },
    effects: {
      opacity: 100,
      shadow: {
        enabled: false,
        color: '#000000',
        blur: 10,
        offsetX: 0,
        offsetY: 4
      }
    },
    visible: true,
    locked: false,
    zIndex: 1
  }
}

export function createDefaultImage(id: string): UIElement {
  return {
    id,
    type: 'image',
    name: 'Imagem',
    x: 100,
    y: 100,
    width: 100,
    height: 100,
    text: '',
    backgroundImage: '',
    effects: {
      opacity: 100,
      borderRadius: 0
    },
    visible: true,
    locked: false,
    zIndex: 2
  }
}

export function createDefaultWebview(id: string): UIElement {
  return {
    id,
    type: 'webview',
    name: 'WebView',
    x: 50,
    y: 50,
    width: 300,
    height: 200,
    text: '',
    webviewConfig: {
      url: 'https://example.com',
      borderRadius: 8,
      borderColor: '#333333',
      borderWidth: 1,
      backgroundColor: '#1e1e1e'
    },
    effects: {
      opacity: 100,
      borderRadius: 8
    },
    visible: true,
    locked: false,
    zIndex: 3
  }
}
