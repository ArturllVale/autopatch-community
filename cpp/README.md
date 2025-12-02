# AutoPatch Community - C++ Native Version

Um sistema de autopatcher para Ragnarok Online escrito em C++ nativo, inspirado no eLuair/rPatchur.

## Características

- **Sem dependências de runtime**: Não requer .NET, Java ou outras dependências
- **Configuração embutida no EXE**: O Builder gera um executável único com todas as configurações e skin
- **Dois modos de UI**:
  - **Image Mode**: Interface skinnable com imagem de fundo e botões posicionáveis
  - **HTML Mode**: Interface usando HTML/CSS/JS para máxima customização
- **Suporte a GRF/THOR**: Compatível com arquivos de patch do Ragnarok Online
- **Download HTTP/HTTPS**: Usa WinHTTP nativo do Windows

## Estrutura do Projeto

```
cpp/
├── CMakeLists.txt          # Configuração do build
├── src/
│   ├── core/               # Biblioteca core
│   │   ├── config.h/cpp    # Estruturas de configuração
│   │   ├── grf.h/cpp       # Parser de arquivos GRF
│   │   ├── thor.h/cpp      # Parser de arquivos THOR
│   │   ├── http.h/cpp      # Cliente HTTP (WinHTTP)
│   │   ├── patcher.h/cpp   # Lógica de patching
│   │   ├── resources.h/cpp # Manipulação de recursos Win32
│   │   └── utils.h/cpp     # Funções utilitárias
│   ├── client/             # Aplicação cliente (Patcher)
│   │   ├── main.cpp        # Entry point
│   │   ├── window.h/cpp    # Janela principal Win32
│   │   ├── ui.h/cpp        # Componentes de UI (GDI+)
│   │   ├── skin.h/cpp      # Carregamento de skin
│   │   └── resources.rc    # Recursos do executável
│   └── builder/            # Aplicação builder
│       ├── main.cpp        # Entry point
│       ├── window.h/cpp    # Interface do builder
│       ├── embedder.h/cpp  # Embutir config no EXE
│       └── resources.rc    # Recursos do executável
└── README.md
```

## Requisitos para Build

- Windows 10 ou superior
- Visual Studio 2022 ou CMake 3.20+
- Git (para baixar dependências)

## Como Compilar

### Usando Visual Studio

1. Abra o Visual Studio 2022
2. Selecione "Open a local folder" e escolha a pasta `cpp`
3. O Visual Studio detectará o CMakeLists.txt automaticamente
4. Selecione a configuração desejada (Debug/Release)
5. Build → Build All

### Usando CMake (linha de comando)

```powershell
# Navega para a pasta do projeto
cd cpp

# Cria pasta de build
mkdir build
cd build

# Configura o projeto
cmake .. -G "Visual Studio 17 2022" -A x64

# Compila
cmake --build . --config Release

# Os executáveis estarão em build/bin/Release/
```

### Dependências Automáticas

O CMake baixará automaticamente:

- **nlohmann/json**: Para parsing de JSON
- **zlib**: Para descompressão de arquivos GRF

## Como Usar

### 1. Compile o projeto

Compile tanto `AutoPatcher.exe` (cliente) quanto `AutoPatchBuilder.exe` (builder).

### 2. Use o Builder

1. Execute `AutoPatchBuilder.exe`
2. Configure as informações do seu servidor:
   - Server Name
   - Patch List URL
   - GRF files
   - etc.
3. Escolha o modo de UI (Image ou HTML)
4. Configure o skin/template
5. Selecione o `AutoPatcher.exe` como template
6. Escolha onde salvar o patcher final
7. Clique em "Build Patcher"

### 3. Distribua

O executável gerado contém todas as configurações e skin embutidos.
Basta distribuir o único arquivo `.exe` para seus usuários.

## Configuração do Servidor de Patches

O patcher espera um arquivo de lista de patches no formato:

```
// plist.txt
// Formato: arquivo_thor|tamanho_bytes|hash_md5
patch_001.thor|123456|abc123def456...
patch_002.thor|789012|789abc012def...
```

Os arquivos `.thor` devem estar disponíveis no mesmo servidor.

## Image Mode

Para usar o Image Mode:

1. Crie uma imagem de fundo (PNG/JPG)
2. Crie imagens para os botões (normal, hover, pressed)
3. Configure as posições dos elementos no Builder

### Elementos configuráveis:

- Botões (start_game, check_files, settings, minimize, close)
- Labels (status, version, etc.)
- Barra de progresso

## HTML Mode

Para usar o HTML Mode:

1. Crie arquivos HTML, CSS e JS
2. Use IDs específicos para os elementos interativos
3. O patcher injeta JavaScript para comunicação

### IDs padrão:

- `btn-start`: Botão de iniciar
- `btn-close`: Botão de fechar
- `btn-minimize`: Botão de minimizar
- `progress-bar`: Barra de progresso
- `status-text`: Texto de status

### API JavaScript disponível:

```javascript
// O patcher expõe estas funções:
window.patcher.startGame();
window.patcher.checkFiles();
window.patcher.minimize();
window.patcher.close();

// Callbacks que o patcher chama:
window.onPatcherStatus = function (status) {};
window.onPatcherProgress = function (percent) {};
window.onPatcherComplete = function () {};
window.onPatcherError = function (message) {};
```

## Licença

MIT License - Veja o arquivo LICENSE para detalhes.

## Créditos

- Inspirado no [rPatchur/eLuair](https://github.com/L1nkZ/rpatchur)
- Formato GRF/THOR baseado na documentação da comunidade Ragnarok
