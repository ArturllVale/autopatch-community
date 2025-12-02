# AutoPatch Builder - Plano de Desenvolvimento

## Visão Geral

Migração do Builder de C++ Win32/GDI+ para **Electron + Vue 3** com TypeScript, mantendo a lógica de geração de EXE em C++ via CLI.

## Arquitetura

```
┌─────────────────────────────────────────────────────────────┐
│                    Electron App                             │
├─────────────────────────────────────────────────────────────┤
│  Renderer Process (Vue 3 + TypeScript)                      │
│  ├── Componentes Vue                                        │
│  │   ├── DragDropCanvas.vue    # Editor visual              │
│  │   ├── PropertyPanel.vue     # Painel de propriedades     │
│  │   ├── Toolbar.vue           # Barra de ferramentas       │
│  │   ├── CodeEditor.vue        # Monaco Editor (HTML/CSS/JS)│
│  │   └── Preview.vue           # Preview em tempo real      │
│  ├── Stores (Pinia)                                         │
│  │   ├── projectStore.ts       # Estado do projeto          │
│  │   └── uiStore.ts            # Estado da UI               │
│  └── IPC para Main Process                                  │
├─────────────────────────────────────────────────────────────┤
│  Main Process (Node.js)                                     │
│  ├── Gerenciamento de janela                                │
│  ├── Diálogos nativos (abrir/salvar arquivo)                │
│  ├── File System (salvar/carregar projetos)                 │
│  └── Spawn do CLI C++ para gerar EXE                        │
├─────────────────────────────────────────────────────────────┤
│  CLI C++ (embedder.exe)                                     │
│  ├── Recebe JSON de configuração                            │
│  ├── Embute config como recurso Win32                       │
│  ├── Embute imagem de background                            │
│  └── Gera patcher.exe final                                 │
└─────────────────────────────────────────────────────────────┘
```

## Stack Tecnológico

| Componente | Tecnologia | Motivo |
|------------|------------|--------|
| Framework Desktop | Electron 28+ | Cross-platform, integração Node.js |
| UI Framework | Vue 3 + Composition API | Reatividade, componentes modulares |
| Linguagem | TypeScript | Type safety, melhor DX |
| Build Tool | Vite + electron-vite | Build rápido, HMR |
| Estado | Pinia | Store oficial Vue 3 |
| Drag & Drop | VueDraggable / vue-draggable-plus | Maduro, bem documentado |
| Editor Código | Monaco Editor | Mesmo do VS Code |
| Estilização | Tailwind CSS | Utility-first, tema escuro |
| Geração EXE | C++ CLI (embedder.exe) | Win32 UpdateResource API |

## Estrutura de Pastas

```
electron-builder/
├── package.json
├── electron.vite.config.ts
├── tsconfig.json
├── tailwind.config.js
├── src/
│   ├── main/                    # Electron Main Process
│   │   ├── index.ts             # Entry point
│   │   ├── ipc-handlers.ts      # IPC handlers
│   │   └── native/              # Integração com C++
│   │       └── embedder.ts      # Wrapper para CLI
│   ├── preload/
│   │   └── index.ts             # Preload script (IPC bridge)
│   └── renderer/                # Vue App
│       ├── index.html
│       ├── main.ts              # Vue entry
│       ├── App.vue
│       ├── components/
│       │   ├── layout/
│       │   │   ├── Sidebar.vue
│       │   │   ├── Toolbar.vue
│       │   │   └── StatusBar.vue
│       │   ├── editor/
│       │   │   ├── DesignCanvas.vue
│       │   │   ├── DraggableElement.vue
│       │   │   ├── PropertyPanel.vue
│       │   │   └── CodeEditor.vue
│       │   └── dialogs/
│       │       ├── ExportDialog.vue
│       │       └── SettingsDialog.vue
│       ├── stores/
│       │   ├── project.ts
│       │   └── ui.ts
│       ├── types/
│       │   └── index.ts
│       ├── utils/
│       │   └── ipc.ts
│       └── assets/
│           └── styles/
│               └── main.css
├── native/                      # CLI C++ para embedding
│   ├── CMakeLists.txt
│   ├── src/
│   │   └── main.cpp             # CLI entry point
│   └── build/
└── resources/                   # Recursos do Electron
    └── icon.ico
```

## Fases de Desenvolvimento

### Fase 1: Setup do Projeto (Dia 1)
- [x] Criar estrutura do projeto Electron + Vue
- [ ] Configurar electron-vite
- [ ] Configurar Tailwind CSS
- [ ] Configurar TypeScript
- [ ] Criar layout base (sidebar, toolbar, main area)

### Fase 2: Editor Visual (Dias 2-3)
- [ ] Implementar DesignCanvas com grid
- [ ] Implementar drag & drop de elementos
- [ ] Tipos de elementos: Button, Label, ProgressBar, StatusLabel, PercentLabel
- [ ] Painel de propriedades reativo
- [ ] Seleção e edição de elementos
- [ ] Carregar imagem de background
- [ ] Redimensionar elementos

### Fase 3: Editor de Código (Dia 4)
- [ ] Integrar Monaco Editor
- [ ] Tabs para HTML, CSS, JavaScript
- [ ] Syntax highlighting
- [ ] Preview do HTML mode

### Fase 4: Gerenciamento de Projeto (Dia 5)
- [ ] Salvar/carregar projetos (.approject JSON)
- [ ] Configurações do servidor (URLs, GRFs)
- [ ] Histórico de projetos recentes

### Fase 5: Geração de EXE (Dia 6)
- [ ] Criar CLI C++ (embedder.exe)
- [ ] IPC para chamar CLI do Electron
- [ ] Diálogo de exportação
- [ ] Progress de geração
- [ ] Validação de configuração

### Fase 6: Polish (Dia 7)
- [ ] Tema escuro completo
- [ ] Atalhos de teclado
- [ ] Undo/Redo
- [ ] Mensagens de erro amigáveis
- [ ] Testes básicos

## Tipos TypeScript

```typescript
// types/index.ts

export type ElementType = 'button' | 'label' | 'progress' | 'status' | 'percentage';

export interface UIElement {
  id: string;
  type: ElementType;
  x: number;
  y: number;
  width: number;
  height: number;
  text: string;
  action?: string;       // Para botões
  fontName?: string;
  fontSize?: number;
  fontColor?: string;
  backgroundColor?: string;
}

export interface ProjectConfig {
  // Informações do servidor
  serverName: string;
  patchListUrl: string;
  newsUrl?: string;
  
  // Cliente
  clientExe: string;
  clientArgs?: string;
  grfFiles: string[];
  
  // UI
  windowWidth: number;
  windowHeight: number;
  uiMode: 'image' | 'html';
  
  // Elementos (modo imagem)
  elements: UIElement[];
  backgroundImage?: string;  // Path ou base64
  
  // HTML mode
  htmlContent?: string;
  cssContent?: string;
  jsContent?: string;
}

export interface Project {
  name: string;
  path?: string;
  config: ProjectConfig;
  isDirty: boolean;
}
```

## IPC Channels

| Channel | Direction | Payload | Descrição |
|---------|-----------|---------|-----------|
| `dialog:open-file` | renderer → main | `{ filters, title }` | Abre diálogo de arquivo |
| `dialog:save-file` | renderer → main | `{ filters, title, defaultPath }` | Abre diálogo salvar |
| `project:save` | renderer → main | `{ path, data }` | Salva projeto |
| `project:load` | renderer → main | `{ path }` | Carrega projeto |
| `build:generate-exe` | renderer → main | `{ config, templatePath, outputPath }` | Gera EXE |
| `build:progress` | main → renderer | `{ percent, message }` | Progresso da geração |

## CLI C++ (embedder.exe)

### Uso
```bash
embedder.exe --config config.json --template AutoPatcher.exe --output MyPatcher.exe [--background bg.png]
```

### Argumentos
| Argumento | Obrigatório | Descrição |
|-----------|-------------|-----------|
| `--config` | Sim | Caminho para JSON de configuração |
| `--template` | Sim | Caminho para EXE template |
| `--output` | Sim | Caminho para EXE de saída |
| `--background` | Não | Imagem de background (PNG/JPG) |
| `--icon` | Não | Ícone do EXE (ICO) |

### Códigos de Saída
| Código | Significado |
|--------|-------------|
| 0 | Sucesso |
| 1 | Erro de argumentos |
| 2 | Arquivo não encontrado |
| 3 | Erro ao copiar template |
| 4 | Erro ao embutir recursos |
| 5 | Erro de JSON inválido |

## Comandos de Desenvolvimento

```bash
# Instalar dependências
npm install

# Desenvolvimento com hot reload
npm run dev

# Build para produção
npm run build

# Build do CLI C++
cd native && cmake -B build && cmake --build build --config Release
```

## Dependências Principais

```json
{
  "dependencies": {
    "vue": "^3.4.0",
    "pinia": "^2.1.0",
    "vue-draggable-plus": "^0.5.0",
    "@guolao/vue-monaco-editor": "^1.5.0"
  },
  "devDependencies": {
    "electron": "^28.0.0",
    "electron-vite": "^2.0.0",
    "vite": "^5.0.0",
    "typescript": "^5.3.0",
    "tailwindcss": "^3.4.0",
    "vue-tsc": "^1.8.0"
  }
}
```

## Próximos Passos

1. ✅ Criar plano de desenvolvimento
2. 🔄 Inicializar projeto com electron-vite
3. ⏳ Configurar estrutura base
4. ⏳ Implementar layout principal
5. ⏳ Implementar editor visual
