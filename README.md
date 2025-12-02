# AutoPatch Community

Um sistema de autopatcher para Ragnarok Online com configuração embutida no EXE.

## Arquitetura

O projeto usa uma arquitetura híbrida:

- **Builder** (C# WPF .NET 8): Interface visual rica para criar e configurar o patcher
- **Patcher** (C++ Win32/GDI+): Executável nativo leve para os usuários finais

## Características

- 📦 Configuração embutida no EXE (sem INI/JSON externo)
- 🎨 Interface customizável via imagem ou HTML/CSS/JS
- 🔄 Suporte a formatos GRF/THOR para patches
- 🛠️ Editor visual drag-and-drop para posicionar elementos
- 🚀 Executável nativo pequeno (~500KB) sem dependências
- 🖥️ Suporte a Windows 7/8/10/11

## Estrutura do Projeto

```
src/
├── Builder/                 # Ferramenta visual (C# WPF)
│   ├── Views/               # MainWindow, ExportWindow
│   ├── Controls/            # ImageModeEditor, HtmlModeEditor
│   ├── Models/              # PatcherProject, UIElements
│   ├── Services/            # PatcherGenerator
│   └── Themes/              # Dark theme resources

cpp/
├── src/
│   ├── core/                # Biblioteca compartilhada
│   │   ├── config.h/cpp     # Carregador de configuração
│   │   ├── grf.h/cpp        # Parser GRF
│   │   ├── thor.h/cpp       # Parser THOR
│   │   ├── http.h/cpp       # Download HTTP
│   │   ├── patcher.h/cpp    # Lógica de patching
│   │   └── utils.h/cpp      # Utilitários
│   ├── client/              # AutoPatcher.exe
│   │   ├── main.cpp
│   │   ├── window.h/cpp     # Janela principal
│   │   ├── ui.h/cpp         # Renderização GDI+
│   │   └── skin.h/cpp       # Sistema de skin
│   └── builder/             # AutoPatchBuilder.exe (C++)
└── CMakeLists.txt
```

## Como Usar

### Compilando o Builder (C#)

```bash
cd src/Builder
dotnet build
```

### Compilando o Patcher (C++)

Requer Visual Studio 2022 com C++ Desktop Development.

```bash
cd cpp/build
cmake --build . --config Release
```

### Usando o Builder

1. Execute `AutoPatchBuilder.exe`
2. Configure as URLs do servidor de patches
3. Escolha o modo de interface (Imagem ou HTML)
4. No modo Imagem:
   - Selecione uma imagem de fundo
   - Adicione botões, labels e barra de progresso
   - Arraste para posicionar os elementos
5. Clique em "Gerar Patcher EXE"
6. O EXE gerado contém toda a configuração embutida

## Configuração do Servidor

1. Configure um servidor HTTP para servir os arquivos de patch
2. Crie o arquivo `patchlist.txt` com a lista de patches
3. Coloque os arquivos `.thor` na pasta de patches

### Formato do Patchlist

```
# Comentários começam com #
# ID FILENAME
1 patch001.thor
2 patch002.thor
3 patch003.thor
```

## Ações dos Botões

Configure a propriedade "Action" dos botões:

- `start_game` - Inicia o executável do jogo
- `check_files` - Verifica arquivos e baixa patches
- `settings` - Abre configurações (futuro)
- `close` - Fecha o patcher
- `minimize` - Minimiza a janela
- `url:https://...` - Abre URL no navegador

- Versões: 1.02, 1.03, 2.00, 3.00
- Compressão: ZLIB
- Criptografia: DES (v1.x apenas)

### GPF (Gravity Patch File)

- Mesmo formato do GRF
- Usado para patches incrementais

### THOR

- Formato otimizado para patches
- Suporta remoção de arquivos
- Magic: "ASSF (C) 2007 Aeomin DEV"

### RGZ

- Arquivo GZIP contendo estrutura de diretórios
- Usado para patches que extraem para pasta

## Licença

MIT License - Veja LICENSE para detalhes.

## Créditos

- **Cremané** (saadrcaa@gmail.com) - Contribuidor e mantenedor
- Documentação GRF baseada em GRF Editor Internals
- Inspirado em Thor Patcher e rPatchur
