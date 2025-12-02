# AutoPatch Community - Plano de Implementação

Sistema de AutoPatcher para Ragnarok Online desenvolvido em C# com suporte a GRF, GPF, RGZ e THOR.

---

## 📋 Índice

1. [Visão Geral](#1-visão-geral)
2. [Arquitetura do Sistema](#2-arquitetura-do-sistema)
3. [Formato do PatchList](#3-formato-do-patchlist)
4. [Estrutura do Projeto](#4-estrutura-do-projeto)
5. [AutoPatch.Core - Biblioteca Principal](#5-autopatchcore---biblioteca-principal)
6. [AutoPatch.Client - O Patcher](#6-autopatchclient---o-patcher)
7. [AutoPatch.Builder - Configurador](#7-autopatchbuilder---configurador)
8. [Fluxo de Patching](#8-fluxo-de-patching)
9. [Auto-Atualização do Patcher](#9-auto-atualização-do-patcher)
10. [Customização Visual](#10-customização-visual)
11. [Configurações](#11-configurações)
12. [Cronograma de Desenvolvimento](#12-cronograma-de-desenvolvimento)
13. [Referências Técnicas](#13-referências-técnicas)

---

## 1. Visão Geral

### 1.1 Objetivo

Criar um sistema de autopatcher completo e customizável para Ragnarok Online que:

- ✅ Suporte múltiplos formatos de patch (GRF, GPF, RGZ, THOR)
- ✅ Seja visualmente customizável (imagens, cores, fontes)
- ✅ Tenha um Builder para configurar e gerar o patcher final
- ✅ Use um formato de patchlist simples e legível
- ✅ Seja eficiente (QuickMerge, não reescreve GRF inteiro)
- ✅ Suporte HTTPS e múltiplos mirrors
- ✅ **Auto-atualização do próprio patcher (executável e recursos)**
- ✅ Seja open-source e bem documentado

### 1.2 Componentes

| Componente            | Descrição                                 |
| --------------------- | ----------------------------------------- |
| **AutoPatch.Core**    | Biblioteca com lógica de GRF/GPF/RGZ/THOR |
| **AutoPatch.Client**  | Aplicação do patcher (WPF/Avalonia)       |
| **AutoPatch.Builder** | Configurador visual para gerar o patcher  |
| **AutoPatch.Updater** | Mini-aplicação para atualizar o patcher   |

### 1.3 Tecnologias

- **Linguagem:** C# (.NET 8)
- **UI Framework:** WPF (Windows) ou Avalonia (Cross-platform)
- **Compressão:** System.IO.Compression (ZLIB/GZIP)
- **HTTP:** HttpClient com suporte a resumo de download
- **Criptografia:** DES para GRFs legados

---

## 2. Arquitetura do Sistema

### 2.1 Diagrama de Componentes

```
┌─────────────────────────────────────────────────────────────────┐
│                      SERVIDOR (HTTP/HTTPS)                       │
├─────────────────────────────────────────────────────────────────┤
│  /patch/                                                         │
│  ├── patchlist.txt          # Lista de patches                  │
│  ├── patcher_version.json   # Versão atual do patcher           │
│  ├── news.html              # Notícias (opcional)               │
│  ├── patcher/               # Arquivos do patcher               │
│  │   ├── autopatcher.exe    # Executável atualizado             │
│  │   ├── updater.exe        # Mini-updater                      │
│  │   └── resources.pak      # Recursos visuais (imagens, etc)   │
│  ├── patches/                                                    │
│  │   ├── 001_base.grf                                           │
│  │   ├── 002_update.thor                                        │
│  │   ├── 003_hotfix.rgz                                         │
│  │   └── 004_data.gpf                                           │
│  └── client/                                                     │
│      └── autopatcher.exe    # Patcher para download             │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ HTTP/HTTPS
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                     CLIENTE (AutoPatch.Client)                   │
├─────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │   UI Layer   │  │ Patch Engine │  │  GRF Engine  │          │
│  │    (WPF)     │──│   Download   │──│  Read/Write  │          │
│  │              │  │    Apply     │  │    Merge     │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│                              │                                   │
│                              ▼                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    AutoPatch.Core                        │   │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐       │   │
│  │  │ GrfLib  │ │ GpfLib  │ │ RgzLib  │ │ ThorLib │       │   │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘       │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      PASTA DO CLIENTE RO                         │
├─────────────────────────────────────────────────────────────────┤
│  ├── data.grf               # GRF principal                     │
│  ├── rdata.grf              # GRF de recursos                   │
│  ├── autopatcher.exe        # Patcher principal                 │
│  ├── updater.exe            # Mini-updater (sempre presente)    │
│  ├── resources/             # Recursos externos (opcional)      │
│  │   ├── background.png                                         │
│  │   ├── logo.png                                               │
│  │   └── buttons/                                               │
│  ├── autopatcher.dat        # Cache de versão local             │
│  └── Ragexe.exe             # Executável do jogo                │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Fluxo de Dados

```
[Patchlist] ──> [Download Manager] ──> [Patch Processor] ──> [GRF Engine]
                      │                       │                    │
                      ▼                       ▼                    ▼
               [Progress UI]          [File Extractor]      [QuickMerge]
```

---

## 3. Formato do PatchList

### 3.1 Formato Simplificado

O patchlist usa um formato simples e legível:

```
// patchlist.txt
// Formato: ID ARQUIVO [OPÇÕES]
// Comentários começam com //

1 base_data.grf
2 update_20231115.thor
3 hotfix_001.rgz
4 custom_sprites.gpf
5 event_natal.thor target=event.grf
6 client_update.thor extract=true
```

### 3.2 Especificação

| Campo      | Descrição                      | Obrigatório |
| ---------- | ------------------------------ | ----------- |
| `ID`       | Número sequencial do patch     | Sim         |
| `ARQUIVO`  | Nome do arquivo de patch       | Sim         |
| `target=`  | GRF alvo (padrão: data.grf)    | Não         |
| `extract=` | Extrair para disco (não merge) | Não         |
| `hash=`    | SHA256 para verificação        | Não         |
| `size=`    | Tamanho em bytes               | Não         |

### 3.3 Exemplo Completo

```
// AutoPatch Community - PatchList
// Servidor: MeuRO
// Última atualização: 2024-11-30

// Patches Base
1 base_data_v1.grf hash=abc123... size=524288000

// Atualizações
2 update_2024_01.thor
3 update_2024_02.thor
4 update_2024_03.thor

// Hotfixes
5 hotfix_skill_fix.rgz extract=true
6 hotfix_map_crash.thor

// Customizações
7 custom_ui.gpf target=custom.grf
8 custom_sprites.thor target=custom.grf

// Evento Especial
9 evento_natal_2024.thor target=evento.grf
```

### 3.4 Arquivo de Cache Local (autopatcher.dat)

```json
{
  "lastPatchId": 8,
  "installedPatches": [1, 2, 3, 4, 5, 6, 7, 8],
  "grfVersions": {
    "data.grf": "2024-11-30T10:30:00Z",
    "custom.grf": "2024-11-28T15:00:00Z"
  },
  "lastCheck": "2024-11-30T12:00:00Z"
}
```

---

## 4. Estrutura do Projeto

### 4.1 Solução Visual Studio

```
AutoPatchCommunity/
├── AutoPatchCommunity.sln
│
├── src/
│   ├── AutoPatch.Core/                    # Biblioteca principal
│   │   ├── AutoPatch.Core.csproj
│   │   ├── Formats/
│   │   │   ├── Grf/
│   │   │   │   ├── GrfArchive.cs          # Leitura/escrita de GRF
│   │   │   │   ├── GrfEntry.cs            # Entrada de arquivo
│   │   │   │   ├── GrfHeader.cs           # Header do GRF
│   │   │   │   └── GrfConstants.cs        # Constantes
│   │   │   ├── Thor/
│   │   │   │   ├── ThorArchive.cs         # Leitura de THOR
│   │   │   │   ├── ThorEntry.cs           # Entrada THOR
│   │   │   │   └── ThorHeader.cs          # Header THOR
│   │   │   ├── Rgz/
│   │   │   │   ├── RgzArchive.cs          # Leitura de RGZ
│   │   │   │   └── RgzEntry.cs            # Entrada RGZ
│   │   │   └── PatchFile.cs               # Abstração de patch
│   │   ├── Crypto/
│   │   │   ├── DesEncryption.cs           # Criptografia DES
│   │   │   └── Crc32.cs                   # CRC32 para verificação
│   │   ├── Compression/
│   │   │   ├── ZlibHelper.cs              # Compressão ZLIB
│   │   │   └── GzipHelper.cs              # Compressão GZIP
│   │   ├── Patching/
│   │   │   ├── PatchEngine.cs             # Motor de patching
│   │   │   ├── PatchList.cs               # Parser do patchlist
│   │   │   ├── PatchEntry.cs              # Entrada do patchlist
│   │   │   └── PatchResult.cs             # Resultado de patch
│   │   └── Utils/
│   │       ├── BinaryReaderEx.cs          # Extensões de leitura
│   │       ├── BinaryWriterEx.cs          # Extensões de escrita
│   │       └── PathHelper.cs              # Utilidades de path
│   │
│   ├── AutoPatch.Client/                  # Aplicação do Patcher
│   │   ├── AutoPatch.Client.csproj
│   │   ├── App.xaml
│   │   ├── App.xaml.cs
│   │   ├── MainWindow.xaml
│   │   ├── MainWindow.xaml.cs
│   │   ├── ViewModels/
│   │   │   ├── MainViewModel.cs
│   │   │   └── SettingsViewModel.cs
│   │   ├── Views/
│   │   │   ├── PatcherView.xaml
│   │   │   └── SettingsView.xaml
│   │   ├── Services/
│   │   │   ├── DownloadService.cs         # Download com progresso
│   │   │   ├── PatchService.cs            # Aplicação de patches
│   │   │   └── ConfigService.cs           # Configurações
│   │   ├── Models/
│   │   │   ├── PatcherConfig.cs           # Configuração do patcher
│   │   │   └── PatchProgress.cs           # Progresso de patch
│   │   ├── Resources/
│   │   │   ├── Images/                    # Imagens customizáveis
│   │   │   ├── Styles/                    # Estilos XAML
│   │   │   └── config.json                # Configuração embarcada
│   │   └── Themes/
│   │       ├── DefaultTheme.xaml
│   │       └── DarkTheme.xaml
│   │
│   └── AutoPatch.Builder/                 # Configurador/Builder
│       ├── AutoPatch.Builder.csproj
│       ├── App.xaml
│       ├── MainWindow.xaml
│       ├── ViewModels/
│       │   ├── BuilderViewModel.cs
│       │   ├── VisualEditorViewModel.cs
│       │   └── ConfigEditorViewModel.cs
│       ├── Views/
│       │   ├── ProjectView.xaml           # Configuração do projeto
│       │   ├── VisualEditorView.xaml      # Editor visual
│       │   ├── ServerConfigView.xaml      # Configuração do servidor
│       │   └── BuildView.xaml             # Build do patcher
│       ├── Services/
│       │   ├── ProjectService.cs          # Gerenciamento de projeto
│       │   ├── BuildService.cs            # Build do executável
│       │   └── ResourceService.cs         # Embarcamento de recursos
│       └── Models/
│           ├── BuilderProject.cs          # Projeto do builder
│           └── VisualConfig.cs            # Configuração visual
│
│   └── AutoPatch.Updater/                 # Mini-aplicação de update
│       ├── AutoPatch.Updater.csproj
│       ├── Program.cs                     # Entry point console
│       └── UpdaterLogic.cs                # Lógica de substituição
│
├── tests/
│   ├── AutoPatch.Core.Tests/
│   │   ├── GrfArchiveTests.cs
│   │   ├── ThorArchiveTests.cs
│   │   └── PatchEngineTests.cs
│   └── AutoPatch.Client.Tests/
│
└── docs/
    ├── GRF_EDITOR_INTERNALS.md
    ├── IMPLEMENTATION_PLAN.md
    └── USER_GUIDE.md
```

---

## 5. AutoPatch.Core - Biblioteca Principal

### 5.1 GrfArchive - Manipulação de GRF/GPF

```csharp
namespace AutoPatch.Core.Formats.Grf;

public class GrfArchive : IDisposable
{
    // Propriedades
    public string FilePath { get; }
    public GrfHeader Header { get; }
    public IReadOnlyDictionary<string, GrfEntry> Entries { get; }
    public bool IsModified { get; }

    // Constantes
    public const int HeaderSize = 46;
    public const string Magic = "Master of Magic";

    // Construtor
    public static GrfArchive Open(string path, bool readOnly = false);
    public static GrfArchive Create(string path, GrfVersion version = GrfVersion.V200);

    // Leitura
    public byte[] GetFileData(string relativePath);
    public byte[] GetFileDataCompressed(string relativePath);
    public bool FileExists(string relativePath);
    public IEnumerable<string> GetFiles(string pattern = "*");

    // Escrita
    public void AddFile(string relativePath, byte[] data);
    public void AddFileCompressed(string relativePath, byte[] compressedData,
                                   int decompressedSize);
    public void RemoveFile(string relativePath);
    public void RenameFile(string oldPath, string newPath);

    // Merge
    public void MergeFrom(GrfArchive other);
    public void MergeFrom(ThorArchive thor);
    public void MergeFrom(RgzArchive rgz);

    // Salvamento
    public void Save(IProgress<PatchProgress> progress = null);
    public void SaveAs(string newPath, IProgress<PatchProgress> progress = null);
    public void QuickSave(); // Apenas adiciona ao final, não reescreve

    // Eventos
    public event EventHandler<PatchProgress> ProgressChanged;
}

public class GrfHeader
{
    public byte MajorVersion { get; set; }  // 1, 2, ou 3
    public byte MinorVersion { get; set; }  // 2, 3, ou 0
    public long FileTableOffset { get; set; }
    public int RealFileCount { get; set; }
    public int Seed { get; set; }

    public GrfVersion Version => (MajorVersion, MinorVersion) switch
    {
        (1, 2) => GrfVersion.V102,
        (1, 3) => GrfVersion.V103,
        (2, 0) => GrfVersion.V200,
        (3, 0) => GrfVersion.V300,
        _ => GrfVersion.Unknown
    };
}

public class GrfEntry
{
    public string RelativePath { get; set; }
    public int SizeCompressed { get; set; }
    public int SizeCompressedAligned { get; set; }
    public int SizeDecompressed { get; set; }
    public GrfEntryFlags Flags { get; set; }
    public long Offset { get; set; }

    // Estado interno
    internal bool IsNew { get; set; }
    internal bool IsModified { get; set; }
    internal bool IsDeleted { get; set; }
    internal byte[] CachedData { get; set; }
}

[Flags]
public enum GrfEntryFlags : byte
{
    None = 0x00,
    File = 0x01,
    EncryptMixed = 0x02,
    EncryptHeader = 0x04,
    Added = 0x08
}

public enum GrfVersion
{
    Unknown,
    V102,  // Alpha com DES
    V103,  // Alpha com DES
    V200,  // Padrão (mais comum)
    V300   // Grande (>4GB)
}
```

### 5.2 ThorArchive - Manipulação de THOR

```csharp
namespace AutoPatch.Core.Formats.Thor;

public class ThorArchive : IDisposable
{
    public const string Magic = "ASSF (C) 2007 Aeomin DEV";

    // Propriedades
    public string FilePath { get; }
    public ThorHeader Header { get; }
    public IReadOnlyList<ThorEntry> Entries { get; }

    // Leitura
    public static ThorArchive Open(string path);
    public byte[] GetFileData(ThorEntry entry);

    // Informações
    public bool UseGrfMerging { get; }
    public string TargetGrf { get; }
    public ThorMode Mode { get; }
}

public class ThorHeader
{
    public bool UseGrfMerging { get; set; }
    public int FileCount { get; set; }
    public ThorMode Mode { get; set; }
    public string TargetGrf { get; set; }
    public int FileTableCompressedSize { get; set; }
    public int FileTableOffset { get; set; }
}

public class ThorEntry
{
    public string RelativePath { get; set; }
    public ThorEntryFlags Flags { get; set; }
    public int Offset { get; set; }
    public int SizeCompressed { get; set; }
    public int SizeDecompressed { get; set; }

    public bool IsRemoved => Flags.HasFlag(ThorEntryFlags.Remove);
}

[Flags]
public enum ThorEntryFlags : byte
{
    None = 0x00,
    File = 0x01,
    Remove = 0x05
}

public enum ThorMode : short
{
    Patch = 0x30,    // Patch normal
    ExeUpdate = 0x21 // Atualização de executável
}
```

### 5.3 RgzArchive - Manipulação de RGZ

```csharp
namespace AutoPatch.Core.Formats.Rgz;

public class RgzArchive : IDisposable
{
    // Propriedades
    public IReadOnlyList<RgzEntry> Entries { get; }

    // Leitura
    public static RgzArchive Open(string path);
    public byte[] GetFileData(RgzEntry entry);
}

public class RgzEntry
{
    public RgzEntryType Type { get; set; }
    public string RelativePath { get; set; }
    public byte[] Data { get; set; }
}

public enum RgzEntryType : byte
{
    Directory = (byte)'d',
    File = (byte)'f',
    End = (byte)'e'
}
```

### 5.4 PatchEngine - Motor de Patching

```csharp
namespace AutoPatch.Core.Patching;

public class PatchEngine
{
    private readonly string _clientPath;
    private readonly PatcherConfig _config;

    public PatchEngine(string clientPath, PatcherConfig config);

    // Aplicar patch
    public async Task<PatchResult> ApplyPatchAsync(
        PatchEntry patch,
        Stream patchStream,
        IProgress<PatchProgress> progress = null,
        CancellationToken cancellationToken = default);

    // Aplicar lista de patches
    public async Task<List<PatchResult>> ApplyPatchListAsync(
        IEnumerable<PatchEntry> patches,
        Func<PatchEntry, Task<Stream>> streamProvider,
        IProgress<PatchProgress> progress = null,
        CancellationToken cancellationToken = default);

    // Verificar integridade
    public async Task<bool> VerifyPatchAsync(
        PatchEntry patch,
        Stream patchStream);
}

public class PatchList
{
    public List<PatchEntry> Entries { get; }

    public static PatchList Parse(string content);
    public static async Task<PatchList> ParseFromUrlAsync(string url);

    public IEnumerable<PatchEntry> GetPendingPatches(int lastPatchId);
}

public class PatchEntry
{
    public int Id { get; set; }
    public string FileName { get; set; }
    public PatchType Type { get; set; }
    public string TargetGrf { get; set; }
    public bool ExtractToDisk { get; set; }
    public string Hash { get; set; }
    public long Size { get; set; }
}

public enum PatchType
{
    Grf,
    Gpf,
    Rgz,
    Thor
}

public class PatchResult
{
    public bool Success { get; set; }
    public int FilesAdded { get; set; }
    public int FilesRemoved { get; set; }
    public int FilesUpdated { get; set; }
    public string ErrorMessage { get; set; }
    public TimeSpan Duration { get; set; }
}

public class PatchProgress
{
    public PatchPhase Phase { get; set; }
    public string CurrentFile { get; set; }
    public int CurrentIndex { get; set; }
    public int TotalCount { get; set; }
    public long BytesProcessed { get; set; }
    public long BytesTotal { get; set; }
    public double ProgressPercent => TotalCount > 0
        ? (double)CurrentIndex / TotalCount * 100
        : 0;
}

public enum PatchPhase
{
    Checking,
    Downloading,
    Extracting,
    Applying,
    Verifying,
    Complete
}
```

---

## 6. AutoPatch.Client - O Patcher

### 6.1 Configuração (config.json embarcado)

```json
{
  "server": {
    "name": "MeuRO",
    "patchUrl": "https://patch.meuro.com.br/",
    "patchListFile": "patchlist.txt",
    "newsUrl": "https://meuro.com.br/news.html",
    "mirrors": [
      "https://mirror1.meuro.com.br/patch/",
      "https://mirror2.meuro.com.br/patch/"
    ]
  },
  "client": {
    "executable": "Ragexe.exe",
    "arguments": [],
    "defaultGrf": "data.grf",
    "grfList": ["data.grf", "rdata.grf"]
  },
  "patching": {
    "inPlace": true,
    "checkIntegrity": true,
    "createGrfIfMissing": true,
    "allowManualPatch": true
  },
  "ui": {
    "title": "MeuRO - AutoPatcher",
    "width": 800,
    "height": 600,
    "resizable": false,
    "theme": "default"
  },
  "visual": {
    "background": "background.png",
    "logo": "logo.png",
    "playButton": {
      "normal": "btn_play.png",
      "hover": "btn_play_hover.png",
      "disabled": "btn_play_disabled.png"
    },
    "progressBar": {
      "background": "#333333",
      "fill": "#00AA00",
      "text": "#FFFFFF"
    },
    "colors": {
      "primary": "#FF6600",
      "secondary": "#333333",
      "text": "#FFFFFF",
      "textSecondary": "#AAAAAA"
    }
  }
}
```

### 6.2 MainWindow.xaml (Exemplo)

```xml
<Window x:Class="AutoPatch.Client.MainWindow"
        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
        Title="{Binding Config.Title}"
        Width="{Binding Config.Width}"
        Height="{Binding Config.Height}"
        WindowStyle="None"
        AllowsTransparency="True"
        ResizeMode="NoResize">

    <Grid>
        <!-- Background -->
        <Image Source="{Binding BackgroundImage}" Stretch="Fill"/>

        <!-- Title Bar -->
        <DockPanel VerticalAlignment="Top" Height="30" Background="#80000000">
            <TextBlock Text="{Binding Config.Title}"
                       Foreground="White"
                       VerticalAlignment="Center"
                       Margin="10,0"/>
            <Button Content="X"
                    DockPanel.Dock="Right"
                    Command="{Binding CloseCommand}"
                    Style="{StaticResource CloseButtonStyle}"/>
            <Button Content="_"
                    DockPanel.Dock="Right"
                    Command="{Binding MinimizeCommand}"
                    Style="{StaticResource MinimizeButtonStyle}"/>
        </DockPanel>

        <!-- Logo -->
        <Image Source="{Binding LogoImage}"
               HorizontalAlignment="Center"
               VerticalAlignment="Top"
               Margin="0,50,0,0"
               MaxHeight="100"/>

        <!-- News Browser -->
        <WebBrowser x:Name="NewsBrowser"
                    Margin="20,160,20,150"
                    Visibility="{Binding ShowNews, Converter={StaticResource BoolToVis}}"/>

        <!-- Progress Section -->
        <StackPanel VerticalAlignment="Bottom" Margin="20,0,20,80">
            <!-- Status Text -->
            <TextBlock Text="{Binding StatusText}"
                       Foreground="{Binding TextColor}"
                       FontSize="12"
                       Margin="0,0,0,5"/>

            <!-- Progress Bar -->
            <Grid Height="25">
                <Border Background="{Binding ProgressBarBackground}"
                        CornerRadius="3"/>
                <Border Background="{Binding ProgressBarFill}"
                        CornerRadius="3"
                        HorizontalAlignment="Left"
                        Width="{Binding ProgressWidth}"/>
                <TextBlock Text="{Binding ProgressText}"
                           Foreground="White"
                           HorizontalAlignment="Center"
                           VerticalAlignment="Center"
                           FontWeight="Bold"/>
            </Grid>

            <!-- File Progress -->
            <TextBlock Text="{Binding CurrentFileText}"
                       Foreground="{Binding TextSecondaryColor}"
                       FontSize="10"
                       Margin="0,5,0,0"/>
        </StackPanel>

        <!-- Buttons -->
        <StackPanel Orientation="Horizontal"
                    HorizontalAlignment="Center"
                    VerticalAlignment="Bottom"
                    Margin="0,0,0,20">

            <!-- Play Button -->
            <Button Command="{Binding PlayCommand}"
                    IsEnabled="{Binding CanPlay}"
                    Style="{StaticResource ImageButtonStyle}">
                <Image Source="{Binding PlayButtonImage}"/>
            </Button>

            <!-- Settings Button -->
            <Button Command="{Binding SettingsCommand}"
                    Style="{StaticResource ImageButtonStyle}"
                    Margin="10,0,0,0">
                <Image Source="{Binding SettingsButtonImage}"/>
            </Button>

        </StackPanel>
    </Grid>
</Window>
```

### 6.3 MainViewModel.cs

```csharp
namespace AutoPatch.Client.ViewModels;

public class MainViewModel : ViewModelBase
{
    private readonly IDownloadService _downloadService;
    private readonly IPatchService _patchService;
    private readonly IConfigService _configService;

    // Propriedades de Estado
    public bool IsPatching { get; private set; }
    public bool CanPlay { get; private set; }
    public string StatusText { get; private set; }
    public double Progress { get; private set; }
    public string ProgressText { get; private set; }
    public string CurrentFileText { get; private set; }

    // Comandos
    public ICommand PlayCommand { get; }
    public ICommand SettingsCommand { get; }
    public ICommand CloseCommand { get; }
    public ICommand MinimizeCommand { get; }
    public ICommand CancelCommand { get; }

    public MainViewModel(
        IDownloadService downloadService,
        IPatchService patchService,
        IConfigService configService)
    {
        _downloadService = downloadService;
        _patchService = patchService;
        _configService = configService;

        PlayCommand = new AsyncRelayCommand(PlayAsync, () => CanPlay);
        // ... outros comandos

        _ = InitializeAsync();
    }

    private async Task InitializeAsync()
    {
        StatusText = "Verificando atualizações...";

        try
        {
            // 1. Baixar patchlist
            var patchList = await _downloadService.GetPatchListAsync();

            // 2. Verificar patches pendentes
            var cache = await _configService.LoadCacheAsync();
            var pending = patchList.GetPendingPatches(cache.LastPatchId).ToList();

            if (pending.Count == 0)
            {
                StatusText = "Cliente atualizado!";
                CanPlay = true;
                return;
            }

            StatusText = $"Baixando {pending.Count} atualizações...";
            IsPatching = true;

            // 3. Baixar e aplicar patches
            var progress = new Progress<PatchProgress>(UpdateProgress);

            foreach (var patch in pending)
            {
                await _patchService.DownloadAndApplyAsync(patch, progress);
                cache.LastPatchId = patch.Id;
                await _configService.SaveCacheAsync(cache);
            }

            StatusText = "Atualização concluída!";
            CanPlay = true;
        }
        catch (Exception ex)
        {
            StatusText = $"Erro: {ex.Message}";
        }
        finally
        {
            IsPatching = false;
        }
    }

    private void UpdateProgress(PatchProgress progress)
    {
        Progress = progress.ProgressPercent;
        ProgressText = $"{progress.ProgressPercent:F1}%";
        CurrentFileText = progress.CurrentFile;

        StatusText = progress.Phase switch
        {
            PatchPhase.Downloading => $"Baixando... ({FormatBytes(progress.BytesProcessed)}/{FormatBytes(progress.BytesTotal)})",
            PatchPhase.Applying => $"Aplicando patch {progress.CurrentIndex}/{progress.TotalCount}...",
            PatchPhase.Verifying => "Verificando integridade...",
            _ => StatusText
        };
    }

    private async Task PlayAsync()
    {
        var config = _configService.GetConfig();
        var exePath = Path.Combine(
            AppContext.BaseDirectory,
            config.Client.Executable);

        Process.Start(new ProcessStartInfo
        {
            FileName = exePath,
            Arguments = string.Join(" ", config.Client.Arguments),
            WorkingDirectory = AppContext.BaseDirectory
        });

        Application.Current.Shutdown();
    }
}
```

---

## 7. AutoPatch.Builder - Configurador

### 7.1 Funcionalidades

O Builder permite configurar e gerar o patcher final:

1. **Configuração do Projeto**

   - Nome do servidor
   - URLs de patch
   - Configurações de cliente

2. **Editor Visual**

   - Selecionar imagem de fundo
   - Selecionar logo
   - Customizar botões
   - Definir cores
   - Preview em tempo real

3. **Configuração do Servidor**

   - URL do patchlist
   - Mirrors
   - Opções de download

4. **Build**
   - Compilar executável
   - Embutir recursos
   - Gerar instalador (opcional)

### 7.2 Interface do Builder

```
┌─────────────────────────────────────────────────────────────────────┐
│  AutoPatch Builder                                    [_][□][X]     │
├─────────────────────────────────────────────────────────────────────┤
│ [Projeto] [Visual] [Servidor] [Build]                               │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌─────────────────────────┐  ┌─────────────────────────────────┐  │
│  │                         │  │         PREVIEW                  │  │
│  │   CONFIGURAÇÕES         │  │  ┌───────────────────────────┐  │  │
│  │                         │  │  │                           │  │  │
│  │   Nome: [MeuRO       ]  │  │  │      [LOGO]               │  │  │
│  │                         │  │  │                           │  │  │
│  │   Largura: [800]        │  │  │  ┌─────────────────────┐  │  │  │
│  │   Altura:  [600]        │  │  │  │                     │  │  │  │
│  │                         │  │  │  │    Notícias         │  │  │  │
│  │   Background:           │  │  │  │                     │  │  │  │
│  │   [bg.png] [Selecionar] │  │  │  └─────────────────────┘  │  │  │
│  │                         │  │  │                           │  │  │
│  │   Logo:                 │  │  │  [========     ] 45%      │  │  │
│  │   [logo.png][Selecionar]│  │  │                           │  │  │
│  │                         │  │  │     [  JOGAR  ]           │  │  │
│  │   Cor Primária:         │  │  │                           │  │  │
│  │   [#FF6600] [...]       │  │  └───────────────────────────┘  │  │
│  │                         │  │                                  │  │
│  └─────────────────────────┘  └─────────────────────────────────┘  │
│                                                                      │
│  [Salvar Projeto]  [Exportar Config]  [Build Patcher]               │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 7.3 BuilderProject.cs

```csharp
namespace AutoPatch.Builder.Models;

public class BuilderProject
{
    public string ProjectName { get; set; }
    public string ProjectPath { get; set; }
    public DateTime LastModified { get; set; }

    // Configuração do Patcher
    public PatcherConfig PatcherConfig { get; set; }

    // Recursos Visuais
    public VisualResources Resources { get; set; }

    // Configuração de Build
    public BuildConfig BuildConfig { get; set; }
}

public class VisualResources
{
    public string BackgroundPath { get; set; }
    public string LogoPath { get; set; }
    public string IconPath { get; set; }

    public ButtonResources PlayButton { get; set; }
    public ButtonResources SettingsButton { get; set; }
    public ButtonResources CloseButton { get; set; }

    public ProgressBarStyle ProgressBar { get; set; }
    public ColorScheme Colors { get; set; }
    public FontSettings Fonts { get; set; }
}

public class ButtonResources
{
    public string NormalPath { get; set; }
    public string HoverPath { get; set; }
    public string PressedPath { get; set; }
    public string DisabledPath { get; set; }
}

public class ColorScheme
{
    public string Primary { get; set; }
    public string Secondary { get; set; }
    public string Background { get; set; }
    public string Text { get; set; }
    public string TextSecondary { get; set; }
    public string Success { get; set; }
    public string Error { get; set; }
}

public class BuildConfig
{
    public string OutputPath { get; set; }
    public string OutputFileName { get; set; }
    public bool SingleFile { get; set; } = true;
    public bool SelfContained { get; set; } = true;
    public string TargetRuntime { get; set; } = "win-x64";
    public bool TrimUnusedCode { get; set; } = true;
    public string Version { get; set; } = "1.0.0";
    public string CompanyName { get; set; }
    public string ProductName { get; set; }
}
```

### 7.4 BuildService.cs

```csharp
namespace AutoPatch.Builder.Services;

public class BuildService : IBuildService
{
    public async Task<BuildResult> BuildPatcherAsync(
        BuilderProject project,
        IProgress<BuildProgress> progress = null)
    {
        var result = new BuildResult();

        try
        {
            // 1. Criar pasta temporária
            var tempDir = Path.Combine(Path.GetTempPath(), $"autopatch_build_{Guid.NewGuid()}");
            Directory.CreateDirectory(tempDir);

            progress?.Report(new BuildProgress("Preparando recursos...", 10));

            // 2. Copiar template do projeto
            CopyProjectTemplate(tempDir);

            // 3. Embutir configuração
            progress?.Report(new BuildProgress("Embutindo configuração...", 20));
            var configJson = JsonSerializer.Serialize(project.PatcherConfig);
            File.WriteAllText(Path.Combine(tempDir, "Resources", "config.json"), configJson);

            // 4. Copiar recursos visuais
            progress?.Report(new BuildProgress("Copiando imagens...", 30));
            CopyVisualResources(project.Resources, Path.Combine(tempDir, "Resources", "Images"));

            // 5. Atualizar propriedades do projeto
            progress?.Report(new BuildProgress("Configurando projeto...", 40));
            UpdateProjectProperties(tempDir, project.BuildConfig);

            // 6. Compilar
            progress?.Report(new BuildProgress("Compilando...", 50));
            var buildOutput = await CompileProjectAsync(tempDir, project.BuildConfig);

            // 7. Copiar para destino
            progress?.Report(new BuildProgress("Finalizando...", 90));
            var outputPath = Path.Combine(
                project.BuildConfig.OutputPath,
                project.BuildConfig.OutputFileName);
            File.Copy(buildOutput, outputPath, true);

            result.Success = true;
            result.OutputPath = outputPath;
            result.FileSize = new FileInfo(outputPath).Length;

            progress?.Report(new BuildProgress("Build concluído!", 100));
        }
        catch (Exception ex)
        {
            result.Success = false;
            result.ErrorMessage = ex.Message;
        }

        return result;
    }

    private async Task<string> CompileProjectAsync(string projectDir, BuildConfig config)
    {
        var psi = new ProcessStartInfo
        {
            FileName = "dotnet",
            Arguments = $"publish -c Release " +
                       $"-r {config.TargetRuntime} " +
                       $"--self-contained {config.SelfContained.ToString().ToLower()} " +
                       $"-p:PublishSingleFile={config.SingleFile.ToString().ToLower()} " +
                       $"-p:PublishTrimmed={config.TrimUnusedCode.ToString().ToLower()}",
            WorkingDirectory = projectDir,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false
        };

        using var process = Process.Start(psi);
        await process.WaitForExitAsync();

        if (process.ExitCode != 0)
        {
            var error = await process.StandardError.ReadToEndAsync();
            throw new BuildException($"Falha na compilação: {error}");
        }

        // Encontrar executável gerado
        var publishDir = Path.Combine(projectDir, "bin", "Release",
            $"net8.0-windows", config.TargetRuntime, "publish");
        return Directory.GetFiles(publishDir, "*.exe").First();
    }
}
```

---

## 8. Fluxo de Patching

### 8.1 Diagrama de Sequência

```
┌─────────┐     ┌─────────┐     ┌─────────┐     ┌─────────┐     ┌─────────┐
│ Cliente │     │Download │     │  Patch  │     │   GRF   │     │Servidor │
│   UI    │     │ Service │     │ Engine  │     │ Archive │     │  HTTP   │
└────┬────┘     └────┬────┘     └────┬────┘     └────┬────┘     └────┬────┘
     │               │               │               │               │
     │ Iniciar       │               │               │               │
     ├──────────────>│               │               │               │
     │               │ GET patchlist.txt             │               │
     │               ├───────────────────────────────────────────────>│
     │               │<──────────────────────────────────────────────┤
     │               │               │               │               │
     │               │ Parse         │               │               │
     │               ├──────────────>│               │               │
     │               │               │               │               │
     │<──────────────┤ Patches pendentes: N         │               │
     │               │               │               │               │
     │ Para cada patch:              │               │               │
     │               │               │               │               │
     │               │ GET patch.thor│               │               │
     │               ├───────────────────────────────────────────────>│
     │               │<──────────────────────────────────────────────┤
     │               │               │               │               │
     │               │ ApplyPatch    │               │               │
     │               ├──────────────>│               │               │
     │               │               │               │               │
     │               │               │ DetectType    │               │
     │               │               ├──────────────>│               │
     │               │               │               │               │
     │               │               │ [THOR] MergeFrom              │
     │               │               ├──────────────>│               │
     │               │               │               │ QuickSave     │
     │               │               │               ├───────┐       │
     │               │               │               │<──────┘       │
     │               │               │<──────────────┤               │
     │               │<──────────────┤ PatchResult   │               │
     │<──────────────┤ Progresso     │               │               │
     │               │               │               │               │
     │ Atualizado!   │               │               │               │
     │<──────────────┤               │               │               │
     │               │               │               │               │
```

### 8.2 Algoritmo de Patching

```csharp
public async Task<PatchResult> ApplyPatchAsync(PatchEntry patch, Stream stream)
{
    var result = new PatchResult();
    var sw = Stopwatch.StartNew();

    // 1. Detectar tipo de patch
    var patchType = DetectPatchType(patch.FileName, stream);

    // 2. Processar de acordo com o tipo
    switch (patchType)
    {
        case PatchType.Thor:
            result = await ApplyThorPatchAsync(stream, patch.TargetGrf);
            break;

        case PatchType.Grf:
        case PatchType.Gpf:
            result = await ApplyGrfPatchAsync(stream, patch.TargetGrf);
            break;

        case PatchType.Rgz:
            result = await ApplyRgzPatchAsync(stream, patch.ExtractToDisk);
            break;
    }

    result.Duration = sw.Elapsed;
    return result;
}

private async Task<PatchResult> ApplyThorPatchAsync(Stream stream, string targetGrf)
{
    var result = new PatchResult();

    using var thor = ThorArchive.Open(stream);

    // Determinar GRF alvo
    var grfName = !string.IsNullOrEmpty(targetGrf)
        ? targetGrf
        : thor.UseGrfMerging
            ? thor.TargetGrf
            : _config.DefaultGrf;

    var grfPath = Path.Combine(_clientPath, grfName);

    // Abrir ou criar GRF
    using var grf = File.Exists(grfPath)
        ? GrfArchive.Open(grfPath)
        : GrfArchive.Create(grfPath);

    // Processar entradas do THOR
    foreach (var entry in thor.Entries)
    {
        if (entry.RelativePath == "data.integrity")
            continue;

        if (entry.IsRemoved)
        {
            grf.RemoveFile(entry.RelativePath);
            result.FilesRemoved++;
        }
        else
        {
            var data = thor.GetFileData(entry);
            grf.AddFileCompressed(entry.RelativePath, data, entry.SizeDecompressed);
            result.FilesAdded++;
        }
    }

    // Salvar com QuickMerge
    grf.QuickSave();

    result.Success = true;
    return result;
}

private async Task<PatchResult> ApplyRgzPatchAsync(Stream stream, bool extractToDisk)
{
    var result = new PatchResult();

    using var rgz = RgzArchive.Open(stream);

    foreach (var entry in rgz.Entries.Where(e => e.Type == RgzEntryType.File))
    {
        var targetPath = Path.Combine(_clientPath, entry.RelativePath);

        if (extractToDisk)
        {
            // Extrair para disco
            Directory.CreateDirectory(Path.GetDirectoryName(targetPath));
            await File.WriteAllBytesAsync(targetPath, entry.Data);
        }
        else
        {
            // Adicionar ao GRF padrão
            using var grf = GrfArchive.Open(
                Path.Combine(_clientPath, _config.DefaultGrf));
            grf.AddFile(entry.RelativePath, entry.Data);
            grf.QuickSave();
        }

        result.FilesAdded++;
    }

    result.Success = true;
    return result;
}
```

---

## 9. Auto-Atualização do Patcher

O patcher verifica sua própria versão **sempre ao iniciar**. Se estiver desatualizado:

- Mostra aviso ao usuário
- Fecha automaticamente
- Abre o Updater para substituir arquivos
- Updater reabre o Patcher atualizado
- Patcher executa o patching normal do jogo

### 9.1 Fluxo Completo de Inicialização

```
┌─────────────────────────────────────────────────────────────────────┐
│                    FLUXO DE INICIALIZAÇÃO DO PATCHER                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ╔═══════════════════════════════════════════════════════════════╗  │
│  ║  1. AutoPatcher.exe INICIA                                    ║  │
│  ╚═══════════════════════════════════════════════════════════════╝  │
│         │                                                            │
│         ▼                                                            │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │  2. GET patcher_version.json do servidor                      │  │
│  │     (verifica versão ANTES de mostrar interface)              │  │
│  └───────────────────────────────────────────────────────────────┘  │
│         │                                                            │
│         ├─── Versão OK ──────────────────────────────────┐          │
│         │                                                 │          │
│         ▼                                                 │          │
│  ┌───────────────────────────────────────────────────┐   │          │
│  │  3. PATCHER DESATUALIZADO!                        │   │          │
│  │                                                   │   │          │
│  │  ┌─────────────────────────────────────────────┐ │   │          │
│  │  │  ⚠️ Atualização do Patcher Necessária!      │ │   │          │
│  │  │                                             │ │   │          │
│  │  │  Sua versão: 1.0.0                         │ │   │          │
│  │  │  Nova versão: 1.2.0                        │ │   │          │
│  │  │                                             │ │   │          │
│  │  │  O patcher será atualizado automaticamente │ │   │          │
│  │  │                                             │ │   │          │
│  │  │  [==========>          ] 45%               │ │   │          │
│  │  │  Baixando: autopatcher.exe                 │ │   │          │
│  │  └─────────────────────────────────────────────┘ │   │          │
│  └───────────────────────────────────────────────────┘   │          │
│         │                                                 │          │
│         ▼                                                 │          │
│  ┌───────────────────────────────────────────────────┐   │          │
│  │  4. Download completo                             │   │          │
│  │     - Salva arquivos em pasta temporária         │   │          │
│  │     - Inicia Updater.exe                         │   │          │
│  │     - FECHA AutoPatcher.exe                      │   │          │
│  └───────────────────────────────────────────────────┘   │          │
│         │                                                 │          │
│         ▼                                                 │          │
│  ╔═══════════════════════════════════════════════════╗   │          │
│  ║  5. UPDATER.EXE (processo separado)               ║   │          │
│  ║                                                   ║   │          │
│  ║  • Aguarda AutoPatcher.exe fechar                 ║   │          │
│  ║  • Substitui arquivos (exe, imagens, config)      ║   │          │
│  ║  • Limpa pasta temporária                         ║   │          │
│  ║  • ABRE AutoPatcher.exe novamente                 ║   │          │
│  ║  • Updater fecha                                  ║   │          │
│  ╚═══════════════════════════════════════════════════╝   │          │
│         │                                                 │          │
│         ▼                                                 │          │
│  ╔═══════════════════════════════════════════════════════════════╗  │
│  ║  6. AutoPatcher.exe REINICIA (agora atualizado)               ║  │
│  ╚═══════════════════════════════════════════════════════════════╝  │
│         │                                                 │          │
│         │◄────────────────────────────────────────────────┘          │
│         ▼                                                            │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │  7. EXECUÇÃO NORMAL DO PATCHER                                │  │
│  │                                                               │  │
│  │  • Carrega interface visual                                   │  │
│  │  • Baixa patchlist.txt                                        │  │
│  │  • Verifica patches pendentes do JOGO                         │  │
│  │  • Baixa e aplica patches (GRF, THOR, RGZ, GPF)              │  │
│  │  • Botão JOGAR habilitado                                     │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 9.2 Arquivo de Versão do Patcher (patcher_version.json)

```json
{
  "version": "1.2.0",
  "buildDate": "2024-11-30T10:00:00Z",
  "minVersion": "1.0.0",
  "forceUpdate": true,
  "files": [
    {
      "name": "autopatcher.exe",
      "hash": "sha256:abc123def456...",
      "size": 5242880,
      "path": "patcher/autopatcher.exe"
    },
    {
      "name": "updater.exe",
      "hash": "sha256:def456ghi789...",
      "size": 102400,
      "path": "patcher/updater.exe"
    }
  ],
  "resources": [
    {
      "name": "background.png",
      "hash": "sha256:111222333...",
      "path": "patcher/resources/background.png"
    },
    {
      "name": "logo.png",
      "hash": "sha256:444555666...",
      "path": "patcher/resources/logo.png"
    },
    {
      "name": "btn_play.png",
      "hash": "sha256:777888999...",
      "path": "patcher/resources/btn_play.png"
    }
  ],
  "changelog": [
    "v1.2.0 - Nova interface visual, correções de bugs",
    "v1.1.0 - Suporte a múltiplos mirrors",
    "v1.0.0 - Versão inicial"
  ],
  "message": ""
}
```

### 9.3 Código do Patcher - Verificação Inicial

```csharp
namespace AutoPatch.Client;

public partial class App : Application
{
    private const string CURRENT_VERSION = "1.0.0";

    protected override async void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        // Verificar se foi iniciado pelo Updater (skip check)
        if (e.Args.Contains("--updated"))
        {
            // Pular verificação, acabou de ser atualizado
            ShowMainWindow();
            return;
        }

        // Mostrar splash de carregamento
        var splash = new SplashWindow();
        splash.Show();
        splash.SetStatus("Verificando versão do patcher...");

        try
        {
            var updateService = new SelfUpdateService();
            var updateCheck = await updateService.CheckVersionAsync();

            if (updateCheck.NeedsUpdate)
            {
                splash.Close();

                // Mostrar janela de atualização obrigatória
                var updateWindow = new PatcherUpdateWindow(updateCheck);
                updateWindow.Show();

                // Iniciar download e depois fechar para Updater
                await updateWindow.DownloadAndLaunchUpdaterAsync();

                // Fechar aplicação (Updater vai reabrir)
                Shutdown();
                return;
            }

            // Versão OK - continuar normalmente
            splash.Close();
            ShowMainWindow();
        }
        catch (Exception ex)
        {
            // Erro na verificação - perguntar se quer continuar
            splash.Close();

            var result = MessageBox.Show(
                $"Não foi possível verificar atualizações:\n{ex.Message}\n\nDeseja continuar mesmo assim?",
                "Aviso",
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning);

            if (result == MessageBoxResult.Yes)
            {
                ShowMainWindow();
            }
            else
            {
                Shutdown();
            }
        }
    }

    private void ShowMainWindow()
    {
        var mainWindow = new MainWindow();
        mainWindow.Show();
    }
}
```

### 9.4 Janela de Atualização do Patcher

```csharp
namespace AutoPatch.Client.Views;

public partial class PatcherUpdateWindow : Window
{
    private readonly UpdateCheckResult _updateInfo;
    private readonly SelfUpdateService _updateService;

    public PatcherUpdateWindow(UpdateCheckResult updateInfo)
    {
        InitializeComponent();
        _updateInfo = updateInfo;
        _updateService = new SelfUpdateService();

        // Exibir informações
        CurrentVersionText.Text = $"Sua versão: {App.CURRENT_VERSION}";
        NewVersionText.Text = $"Nova versão: {updateInfo.ServerVersion}";
        ChangelogText.Text = string.Join("\n", updateInfo.Changelog);
    }

    public async Task DownloadAndLaunchUpdaterAsync()
    {
        var tempFolder = Path.Combine(Path.GetTempPath(), $"autopatch_update_{Guid.NewGuid()}");
        Directory.CreateDirectory(tempFolder);

        try
        {
            // 1. Baixar todos os arquivos
            var allFiles = _updateInfo.Files.Concat(_updateInfo.Resources).ToList();

            for (int i = 0; i < allFiles.Count; i++)
            {
                var file = allFiles[i];

                StatusText.Text = $"Baixando: {file.Name}";
                ProgressBar.Value = (double)i / allFiles.Count * 100;

                await _updateService.DownloadFileAsync(file, tempFolder);
            }

            // 2. Verificar hashes
            StatusText.Text = "Verificando integridade...";
            ProgressBar.Value = 100;

            if (!await _updateService.VerifyFilesAsync(allFiles, tempFolder))
            {
                throw new Exception("Falha na verificação de integridade dos arquivos");
            }

            // 3. Iniciar Updater
            StatusText.Text = "Iniciando atualização...";
            LaunchUpdater(tempFolder);
        }
        catch (Exception ex)
        {
            // Limpar em caso de erro
            TryDeleteDirectory(tempFolder);

            MessageBox.Show(
                $"Erro ao baixar atualização:\n{ex.Message}",
                "Erro",
                MessageBoxButton.OK,
                MessageBoxImage.Error);

            Application.Current.Shutdown();
        }
    }

    private void LaunchUpdater(string tempFolder)
    {
        var currentProcess = Process.GetCurrentProcess();
        var installFolder = AppContext.BaseDirectory;

        // Usar o updater atual (ou o novo se foi baixado)
        var updaterPath = Path.Combine(installFolder, "updater.exe");
        var newUpdater = Path.Combine(tempFolder, "updater.exe");

        if (File.Exists(newUpdater))
        {
            // Copiar novo updater para local temporário separado
            var updaterTemp = Path.Combine(Path.GetTempPath(), "autopatch_updater");
            Directory.CreateDirectory(updaterTemp);
            var tempUpdaterPath = Path.Combine(updaterTemp, "updater.exe");
            File.Copy(newUpdater, tempUpdaterPath, true);
            updaterPath = tempUpdaterPath;
        }

        // Argumentos para o Updater
        var args = new[]
        {
            $"--pid={currentProcess.Id}",
            $"--source=\"{tempFolder}\"",
            $"--target=\"{installFolder}\"",
            $"--exe=autopatcher.exe"
        };

        Process.Start(new ProcessStartInfo
        {
            FileName = updaterPath,
            Arguments = string.Join(" ", args),
            UseShellExecute = true,
            WorkingDirectory = Path.GetDirectoryName(updaterPath)
        });

        // Patcher fecha - Updater assume
    }
}
```

### 9.5 Updater.exe - Código Completo

```csharp
namespace AutoPatch.Updater;

public class Program
{
    public static async Task Main(string[] args)
    {
        Console.Title = "AutoPatch Updater";
        Console.WriteLine("╔════════════════════════════════════════╗");
        Console.WriteLine("║      AutoPatch Community - Updater     ║");
        Console.WriteLine("╚════════════════════════════════════════╝");
        Console.WriteLine();

        var config = ParseArgs(args);

        if (config == null)
        {
            Console.WriteLine("Uso: updater.exe --pid=<pid> --source=<pasta> --target=<pasta> --exe=<nome>");
            Console.WriteLine("\nPressione qualquer tecla para sair...");
            Console.ReadKey();
            return;
        }

        try
        {
            // 1. Aguardar processo do patcher fechar
            Console.WriteLine($"[1/4] Aguardando patcher fechar (PID: {config.ParentPid})...");
            await WaitForProcessExitAsync(config.ParentPid, TimeSpan.FromSeconds(30));
            Console.WriteLine("      ✓ Patcher fechado");

            // 2. Pequeno delay para liberar arquivos
            await Task.Delay(1000);

            // 3. Substituir arquivos
            Console.WriteLine($"[2/4] Atualizando arquivos...");
            var filesUpdated = await CopyFilesAsync(config.SourceFolder, config.TargetFolder);
            Console.WriteLine($"      ✓ {filesUpdated} arquivos atualizados");

            // 4. Limpar pasta temporária
            Console.WriteLine("[3/4] Limpando arquivos temporários...");
            TryDeleteDirectory(config.SourceFolder);
            Console.WriteLine("      ✓ Limpeza concluída");

            // 5. Reiniciar patcher
            Console.WriteLine("[4/4] Reiniciando patcher...");
            var patcherPath = Path.Combine(config.TargetFolder, config.ExeName);

            Process.Start(new ProcessStartInfo
            {
                FileName = patcherPath,
                Arguments = "--updated", // Flag para pular verificação
                WorkingDirectory = config.TargetFolder,
                UseShellExecute = true
            });

            Console.WriteLine("      ✓ Patcher iniciado");
            Console.WriteLine();
            Console.WriteLine("╔════════════════════════════════════════╗");
            Console.WriteLine("║      Atualização concluída com êxito!  ║");
            Console.WriteLine("╚════════════════════════════════════════╝");

            // Fechar após 2 segundos
            await Task.Delay(2000);
        }
        catch (Exception ex)
        {
            Console.WriteLine();
            Console.WriteLine($"[ERRO] {ex.Message}");
            Console.WriteLine();
            Console.WriteLine("Pressione qualquer tecla para sair...");
            Console.ReadKey();
        }
    }

    private static UpdaterConfig? ParseArgs(string[] args)
    {
        var config = new UpdaterConfig();

        foreach (var arg in args)
        {
            if (arg.StartsWith("--pid="))
                config.ParentPid = int.Parse(arg.Substring(6));
            else if (arg.StartsWith("--source="))
                config.SourceFolder = arg.Substring(9).Trim('"');
            else if (arg.StartsWith("--target="))
                config.TargetFolder = arg.Substring(9).Trim('"');
            else if (arg.StartsWith("--exe="))
                config.ExeName = arg.Substring(6).Trim('"');
        }

        if (config.ParentPid == 0 ||
            string.IsNullOrEmpty(config.SourceFolder) ||
            string.IsNullOrEmpty(config.TargetFolder) ||
            string.IsNullOrEmpty(config.ExeName))
        {
            return null;
        }

        return config;
    }

    private static async Task WaitForProcessExitAsync(int pid, TimeSpan timeout)
    {
        try
        {
            var process = Process.GetProcessById(pid);
            using var cts = new CancellationTokenSource(timeout);
            await process.WaitForExitAsync(cts.Token);
        }
        catch (ArgumentException)
        {
            // Processo já fechou - OK
        }
        catch (OperationCanceledException)
        {
            // Timeout - tentar forçar
            try
            {
                Process.GetProcessById(pid)?.Kill();
                await Task.Delay(1000);
            }
            catch { }
        }
    }

    private static async Task<int> CopyFilesAsync(string source, string target)
    {
        int count = 0;

        foreach (var file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
        {
            var relativePath = Path.GetRelativePath(source, file);
            var targetPath = Path.Combine(target, relativePath);

            // Criar diretório se necessário
            var targetDir = Path.GetDirectoryName(targetPath);
            if (!string.IsNullOrEmpty(targetDir))
            {
                Directory.CreateDirectory(targetDir);
            }

            // Tentar copiar com retry (arquivo pode estar em uso)
            for (int attempt = 0; attempt < 5; attempt++)
            {
                try
                {
                    File.Copy(file, targetPath, true);
                    Console.WriteLine($"      → {relativePath}");
                    count++;
                    break;
                }
                catch (IOException) when (attempt < 4)
                {
                    await Task.Delay(500 * (attempt + 1));
                }
            }
        }

        return count;
    }

    private static void TryDeleteDirectory(string path)
    {
        try
        {
            if (Directory.Exists(path))
            {
                Directory.Delete(path, true);
            }
        }
        catch
        {
            // Ignorar erros de limpeza
        }
    }
}

public class UpdaterConfig
{
    public int ParentPid { get; set; }
    public string SourceFolder { get; set; } = "";
    public string TargetFolder { get; set; } = "";
    public string ExeName { get; set; } = "autopatcher.exe";
}
```

### 9.6 SelfUpdateService

```csharp
namespace AutoPatch.Client.Services;

public class SelfUpdateService
{
    private readonly HttpClient _httpClient;
    private readonly string _versionUrl;

    public SelfUpdateService()
    {
        _httpClient = new HttpClient();
        _versionUrl = ConfigService.GetPatcherVersionUrl();
    }

    public async Task<UpdateCheckResult> CheckVersionAsync()
    {
        var result = new UpdateCheckResult();

        // Baixar versão do servidor
        var json = await _httpClient.GetStringAsync(_versionUrl);
        var serverInfo = JsonSerializer.Deserialize<PatcherVersionInfo>(json);

        result.ServerVersion = serverInfo.Version;
        result.Changelog = serverInfo.Changelog;
        result.Files = serverInfo.Files;
        result.Resources = serverInfo.Resources;

        // Comparar versões
        var currentVersion = Version.Parse(App.CURRENT_VERSION);
        var serverVersion = Version.Parse(serverInfo.Version);

        result.NeedsUpdate = serverVersion > currentVersion;
        result.ForceUpdate = serverInfo.ForceUpdate;

        return result;
    }

    public async Task DownloadFileAsync(PatcherFile file, string targetFolder)
    {
        var targetPath = Path.Combine(targetFolder, file.Name);
        var targetDir = Path.GetDirectoryName(targetPath);

        if (!string.IsNullOrEmpty(targetDir))
        {
            Directory.CreateDirectory(targetDir);
        }

        var url = ConfigService.GetPatchBaseUrl() + file.Path;
        var data = await _httpClient.GetByteArrayAsync(url);

        await File.WriteAllBytesAsync(targetPath, data);
    }

    public async Task<bool> VerifyFilesAsync(IEnumerable<PatcherFile> files, string folder)
    {
        foreach (var file in files)
        {
            var filePath = Path.Combine(folder, file.Name);

            if (!File.Exists(filePath))
                return false;

            var hash = await ComputeHashAsync(filePath);
            if (hash != file.Hash)
                return false;
        }

        return true;
    }

    private async Task<string> ComputeHashAsync(string filePath)
    {
        using var sha256 = SHA256.Create();
        using var stream = File.OpenRead(filePath);
        var hash = await sha256.ComputeHashAsync(stream);
        return "sha256:" + Convert.ToHexString(hash).ToLowerInvariant();
    }
}

public class UpdateCheckResult
{
    public bool NeedsUpdate { get; set; }
    public bool ForceUpdate { get; set; }
    public string ServerVersion { get; set; } = "";
    public List<string> Changelog { get; set; } = new();
    public List<PatcherFile> Files { get; set; } = new();
    public List<PatcherFile> Resources { get; set; } = new();
}

public class PatcherFile
{
    public string Name { get; set; } = "";
    public string Hash { get; set; } = "";
    public long Size { get; set; }
    public string Path { get; set; } = "";
}
```

### 9.7 Fluxo Resumido

```
┌──────────────────────────────────────────────────────────────────┐
│                        RESUMO DO FLUXO                           │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  PATCHER INICIA                                                  │
│       │                                                          │
│       ▼                                                          │
│  Verifica patcher_version.json ────────────┐                    │
│       │                                     │                    │
│       │ Desatualizado                       │ Versão OK          │
│       ▼                                     │                    │
│  ┌────────────────────┐                     │                    │
│  │ Mostra aviso       │                     │                    │
│  │ Baixa arquivos     │                     │                    │
│  │ Inicia Updater     │                     │                    │
│  │ FECHA              │                     │                    │
│  └────────────────────┘                     │                    │
│       │                                     │                    │
│       ▼                                     │                    │
│  UPDATER.EXE                                │                    │
│       │                                     │                    │
│       ▼                                     │                    │
│  Substitui arquivos                         │                    │
│       │                                     │                    │
│       ▼                                     │                    │
│  Abre Patcher (--updated)                   │                    │
│       │                                     │                    │
│       └─────────────────────────────────────┤                    │
│                                             │                    │
│                                             ▼                    │
│                                    EXECUÇÃO NORMAL               │
│                                             │                    │
│                                             ▼                    │
│                                    GET patchlist.txt             │
│                                             │                    │
│                                             ▼                    │
│                                    Baixa patches do JOGO         │
│                                    (GRF, THOR, RGZ, GPF)         │
│                                             │                    │
│                                             ▼                    │
│                                    Aplica no data.grf            │
│                                             │                    │
│                                             ▼                    │
│                                    [JOGAR] habilitado            │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### 9.8 Configuração no config.json

```json
{
  "patcher": {
    "version": "1.0.0",
    "versionCheckUrl": "https://patch.meuro.com.br/patcher_version.json",
    "allowOfflineStart": true,
    "showUpdateChangelog": true
  }
}
```

---

## 10. Customização Visual

### 10.1 Sistema de Temas

```csharp
public class ThemeManager
{
    private readonly Dictionary<string, ResourceDictionary> _themes = new();

    public void LoadTheme(string themeName)
    {
        var theme = _themes.GetValueOrDefault(themeName)
            ?? LoadThemeFromResources(themeName);

        Application.Current.Resources.MergedDictionaries.Clear();
        Application.Current.Resources.MergedDictionaries.Add(theme);
    }

    public void ApplyCustomColors(ColorScheme colors)
    {
        var resources = Application.Current.Resources;

        resources["PrimaryColor"] = ColorFromHex(colors.Primary);
        resources["SecondaryColor"] = ColorFromHex(colors.Secondary);
        resources["TextColor"] = ColorFromHex(colors.Text);
        // ... outras cores
    }
}
```

### 10.2 Recursos Embarcados

Os recursos visuais são embarcados como `EmbeddedResource`:

```xml
<!-- AutoPatch.Client.csproj -->
<ItemGroup>
  <EmbeddedResource Include="Resources\Images\**\*" />
  <EmbeddedResource Include="Resources\config.json" />
</ItemGroup>
```

### 10.3 Carregamento de Imagens

```csharp
public class ResourceLoader
{
    public static BitmapImage LoadEmbeddedImage(string resourceName)
    {
        var assembly = Assembly.GetExecutingAssembly();
        var resourcePath = $"AutoPatch.Client.Resources.Images.{resourceName}";

        using var stream = assembly.GetManifestResourceStream(resourcePath);
        if (stream == null)
            return null;

        var image = new BitmapImage();
        image.BeginInit();
        image.StreamSource = stream;
        image.CacheOption = BitmapCacheOption.OnLoad;
        image.EndInit();
        image.Freeze();

        return image;
    }
}
```

---

## 11. Configurações

### 11.1 Arquivo de Configuração do Servidor

```yaml
# server_config.yml (no servidor)
server:
  name: "MeuRO"
  version: "2024.11.30"

patch:
  base_url: "https://patch.meuro.com.br/"
  list_file: "patchlist.txt"

mirrors:
  - name: "Principal"
    url: "https://patch.meuro.com.br/"
    priority: 1
  - name: "Mirror BR"
    url: "https://br.patch.meuro.com.br/"
    priority: 2

news:
  enabled: true
  url: "https://meuro.com.br/patcher/news.html"

maintenance:
  enabled: false
  message: "Servidor em manutenção. Previsão de retorno: 18:00"
```

### 11.2 Cache Local

```csharp
public class LocalCache
{
    public int LastPatchId { get; set; }
    public List<int> InstalledPatches { get; set; } = new();
    public Dictionary<string, GrfInfo> GrfVersions { get; set; } = new();
    public DateTime LastCheck { get; set; }
    public string PatcherVersion { get; set; }
}

public class GrfInfo
{
    public DateTime LastModified { get; set; }
    public long FileSize { get; set; }
    public string Hash { get; set; }
}
```

---

## 12. Cronograma de Desenvolvimento

### Fase 1: Core Library (2-3 semanas)

| Tarefa                          | Estimativa | Prioridade |
| ------------------------------- | ---------- | ---------- |
| Estrutura do projeto            | 2 dias     | Alta       |
| GrfArchive (leitura)            | 3 dias     | Alta       |
| GrfArchive (escrita/QuickMerge) | 4 dias     | Alta       |
| ThorArchive                     | 2 dias     | Alta       |
| RgzArchive                      | 1 dia      | Alta       |
| Compressão ZLIB                 | 1 dia      | Alta       |
| Criptografia DES                | 2 dias     | Média      |
| Testes unitários                | 3 dias     | Alta       |

### Fase 2: Cliente (2-3 semanas)

| Tarefa                  | Estimativa | Prioridade |
| ----------------------- | ---------- | ---------- |
| UI Base (WPF)           | 3 dias     | Alta       |
| Download Service        | 2 dias     | Alta       |
| Patch Service           | 3 dias     | Alta       |
| Progress/Status         | 1 dia      | Alta       |
| **Self-Update Service** | 2 dias     | Alta       |
| **Updater.exe**         | 1 dia      | Alta       |
| Sistema de Temas        | 2 dias     | Média      |
| News Browser            | 1 dia      | Baixa      |
| Settings View           | 1 dia      | Média      |
| Tratamento de erros     | 2 dias     | Alta       |

### Fase 3: Builder (2-3 semanas)

| Tarefa                 | Estimativa | Prioridade |
| ---------------------- | ---------- | ---------- |
| UI do Builder          | 3 dias     | Alta       |
| Editor Visual          | 4 dias     | Alta       |
| Preview em tempo real  | 2 dias     | Média      |
| Sistema de Build       | 3 dias     | Alta       |
| Geração de recursos    | 2 dias     | Alta       |
| Salvamento de projetos | 1 dia      | Alta       |

### Fase 4: Polimento (1-2 semanas)

| Tarefa               | Estimativa | Prioridade |
| -------------------- | ---------- | ---------- |
| Documentação         | 3 dias     | Alta       |
| Testes de integração | 2 dias     | Alta       |
| Otimizações          | 2 dias     | Média      |
| Correção de bugs     | 3 dias     | Alta       |

**Total Estimado: 7-11 semanas**

---

## 13. Referências Técnicas

### 13.1 Projetos de Referência

| Projeto                                                             | Linguagem | Recursos               |
| ------------------------------------------------------------------- | --------- | ---------------------- |
| [GRF Editor (Tokei)](https://github.com/Tokeiburu/GRFEditor)        | C#        | Lógica de GRF/THOR     |
| [rpatchur](https://github.com/L1nkZ/rpatchur)                       | Rust      | Arquitetura de patcher |
| [Thor Patcher](https://rathena.org/board/topic/77080-thor-patcher/) | Delphi    | Formato THOR           |

### 13.2 Especificações

- **GRF Format:** Ver `GRF_EDITOR_INTERNALS.md`
- **THOR Format:** Header + FileTable + Data
- **RGZ Format:** GZIP com estrutura tipo/nome/dados

### 13.3 Bibliotecas .NET Úteis

```xml
<ItemGroup>
  <!-- Compressão -->
  <PackageReference Include="System.IO.Compression" Version="4.3.0" />

  <!-- HTTP -->
  <PackageReference Include="System.Net.Http" Version="4.3.4" />

  <!-- JSON -->
  <PackageReference Include="System.Text.Json" Version="8.0.0" />

  <!-- YAML (para config) -->
  <PackageReference Include="YamlDotNet" Version="15.1.0" />

  <!-- MVVM -->
  <PackageReference Include="CommunityToolkit.Mvvm" Version="8.2.2" />
</ItemGroup>
```

---

## Próximos Passos

1. ✅ Documentação do plano
2. ⬜ Criar estrutura do projeto
3. ⬜ Implementar AutoPatch.Core
4. ⬜ Implementar AutoPatch.Client
5. ⬜ Implementar AutoPatch.Builder
6. ⬜ Testes e documentação final

---

_Documento criado em: 30/11/2024_
_Versão: 1.0_
