# 📘 Guia Completo do AutoPatch Community

**Autor:** Cremané (saadrcaa@gmail.com)  
**Versão:** 1.0.0  
**Data:** Dezembro 2024

---

## 📋 Índice

1. [Introdução](#-introdução)
2. [Requisitos do Sistema](#-requisitos-do-sistema)
3. [Estrutura do Projeto](#-estrutura-do-projeto)
4. [Instalação e Compilação](#-instalação-e-compilação)
5. [Configuração do Builder](#-configuração-do-builder)
6. [Modos de Interface](#-modos-de-interface)
7. [Configuração do Servidor](#-configuração-do-servidor)
8. [Formato dos Arquivos](#-formato-dos-arquivos)
9. [Ações e Botões](#-ações-e-botões)
10. [Solução de Problemas](#-solução-de-problemas)
11. [FAQ](#-faq)

---

## 🎯 Introdução

O **AutoPatch Community** é um sistema de autopatcher profissional para servidores de Ragnarok Online. Ele permite que administradores de servidores criem patchers personalizados com interface visual customizável, sem necessidade de arquivos de configuração externos.

### Características Principais

- 📦 **Configuração Embutida**: Toda configuração fica dentro do EXE
- 🎨 **Interface Customizável**: Suporte a imagem estática ou HTML/CSS/JS
- 🔄 **Múltiplos Formatos**: Suporte a GRF, THOR, GPF e RGZ
- 🛠️ **Editor Visual**: Interface drag-and-drop para posicionar elementos
- 🚀 **Executável Leve**: Aproximadamente 500KB sem dependências externas
- 🖥️ **Compatibilidade**: Windows 7, 8, 10 e 11

---

## 💻 Requisitos do Sistema

### Para Compilar o Projeto

| Componente    | Versão Mínima                    |
| ------------- | -------------------------------- |
| Windows       | 10 ou superior                   |
| Visual Studio | 2022 com C++ Desktop Development |
| CMake         | 3.20+                            |
| .NET SDK      | 8.0 (para Builder C#)            |
| Node.js       | 18+ (para Electron Builder)      |

### Para Usuários Finais

| Componente | Versão Mínima     |
| ---------- | ----------------- |
| Windows    | 7 SP1 ou superior |
| Runtime    | Nenhum necessário |

---

## 📁 Estrutura do Projeto

```
autoPatch Community/
├── cpp/                        # Código C++ nativo
│   ├── src/
│   │   ├── core/               # Biblioteca compartilhada
│   │   │   ├── config.cpp/h    # Sistema de configuração
│   │   │   ├── grf.cpp/h       # Parser de arquivos GRF
│   │   │   ├── thor.cpp/h      # Parser de arquivos THOR
│   │   │   ├── http.cpp/h      # Cliente HTTP para downloads
│   │   │   ├── patcher.cpp/h   # Lógica principal de patching
│   │   │   └── utils.cpp/h     # Funções utilitárias
│   │   ├── client/             # AutoPatcher.exe
│   │   │   ├── main.cpp        # Ponto de entrada
│   │   │   ├── window.cpp/h    # Gerenciamento de janela
│   │   │   ├── ui.cpp/h        # Renderização com GDI+
│   │   │   └── skin.cpp/h      # Sistema de skins
│   │   └── builder/            # AutoPatchBuilder.exe
│   ├── build/                  # Arquivos de build
│   └── CMakeLists.txt          # Configuração CMake
│
├── electron-builder/           # Builder em Electron (alternativo)
│   ├── src/
│   │   ├── main/               # Processo principal
│   │   ├── preload/            # Scripts de preload
│   │   └── renderer/           # Interface Vue.js
│   └── package.json
│
├── src/
│   └── Builder/                # Builder em C# WPF (alternativo)
│
├── samples/                    # Exemplos de configuração
│   ├── patcher.json            # Exemplo de config
│   ├── patchlist.txt           # Exemplo de lista de patches
│   └── version.json            # Exemplo de versão
│
└── doc/                        # Documentação
    └── GUIA_DE_USO.md          # Este arquivo
```

---

## 🔧 Instalação e Compilação

### Compilando o Patcher C++ (Recomendado)

1. **Abra o Developer Command Prompt do Visual Studio 2022**

2. **Navegue até a pasta do projeto**:

   ```powershell
   cd "caminho\para\autoPatch Community\cpp"
   ```

3. **Gere os arquivos de build com CMake**:

   ```powershell
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

4. **Compile o projeto**:

   ```powershell
   cmake --build . --config Release
   ```

5. **Os executáveis estarão em**:
   - `cpp/build/bin/Release/AutoPatcher.exe`
   - `cpp/build/bin/Release/AutoPatchBuilder.exe`

### Compilando o Electron Builder (Alternativo)

1. **Instale as dependências**:

   ```powershell
   cd electron-builder
   npm install
   ```

2. **Execute em modo desenvolvimento**:

   ```powershell
   npm run dev
   ```

3. **Compile para produção**:
   ```powershell
   npm run build:win
   ```

---

## ⚙️ Configuração do Builder

### Janela Principal

Ao abrir o **AutoPatchBuilder.exe**, você verá a interface principal dividida em seções:

#### 1. Configurações do Servidor

| Campo                   | Descrição                                   | Exemplo                                 |
| ----------------------- | ------------------------------------------- | --------------------------------------- |
| **URL Base de Patches** | URL onde os arquivos .thor estão hospedados | `https://seuservidor.com/patches/`      |
| **URL do Patchlist**    | Caminho para o arquivo patchlist.txt        | `https://seuservidor.com/patchlist.txt` |
| **URL de Versão**       | (Opcional) URL para verificar versão        | `https://seuservidor.com/version.json`  |
| **Executável do Jogo**  | Nome do .exe que será iniciado              | `ragexe.exe` ou `Ragnarok.exe`          |

#### 2. Configurações de Arquivos

| Campo                | Descrição                            | Exemplo                     |
| -------------------- | ------------------------------------ | --------------------------- |
| **GRF Principal**    | Nome do arquivo GRF do cliente       | `data.grf`                  |
| **GRF de Patch**     | (Opcional) GRF separado para patches | `rdata.grf`                 |
| **Pasta de Destino** | Pasta onde extrair arquivos          | Deixe vazio para pasta raiz |

#### 3. Configurações de Interface

| Campo                     | Descrição                           |
| ------------------------- | ----------------------------------- |
| **Modo**                  | Escolha entre "Imagem" ou "HTML"    |
| **Título da Janela**      | Texto exibido na barra de título    |
| **Largura/Altura**        | Dimensões da janela do patcher      |
| **Permite Redimensionar** | Se a janela pode ser redimensionada |
| **Mostra na Taskbar**     | Se aparece na barra de tarefas      |

---

## 🎨 Modos de Interface

### Modo Imagem

O modo mais simples, ideal para quem quer um patcher rápido e funcional.

#### Como Configurar:

1. **Selecione uma Imagem de Fundo**

   - Formatos suportados: PNG, JPG, BMP
   - Recomendado: PNG com transparência
   - Tamanho ideal: 600x400 pixels

2. **Adicione Elementos na Interface**

   | Elemento               | Descrição                      |
   | ---------------------- | ------------------------------ |
   | **Botão**              | Elemento clicável com ação     |
   | **Label**              | Texto estático ou dinâmico     |
   | **Barra de Progresso** | Mostra o progresso do download |
   | **Área de Status**     | Exibe mensagens de status      |

3. **Posicione os Elementos**

   - Clique e arraste para mover
   - Use as guias para alinhar
   - Configure tamanho via propriedades

4. **Configure as Propriedades de Cada Elemento**

   **Propriedades do Botão:**
   | Propriedade | Descrição |
   |-------------|-----------|
   | Posição X/Y | Coordenadas na tela |
   | Largura/Altura | Dimensões do botão |
   | Texto | Texto exibido no botão |
   | Ação | O que acontece ao clicar |
   | Imagem Normal | Imagem do botão em repouso |
   | Imagem Hover | Imagem ao passar o mouse |
   | Imagem Pressed | Imagem ao clicar |

   **Propriedades da Barra de Progresso:**
   | Propriedade | Descrição |
   |-------------|-----------|
   | Posição X/Y | Coordenadas na tela |
   | Largura/Altura | Dimensões da barra |
   | Cor de Fundo | Cor da barra vazia |
   | Cor de Preenchimento | Cor da barra cheia |
   | Estilo | Contínuo ou segmentado |

### Modo HTML

Modo avançado que permite criar interfaces ricas com HTML, CSS e JavaScript.

#### Estrutura Básica:

```html
<!DOCTYPE html>
<html>
  <head>
    <style>
      body {
        margin: 0;
        background: url("background.png") no-repeat center;
        font-family: Arial, sans-serif;
      }

      .start-button {
        position: absolute;
        top: 300px;
        left: 200px;
        padding: 15px 40px;
        background: linear-gradient(#4caf50, #45a049);
        border: none;
        color: white;
        cursor: pointer;
        border-radius: 5px;
      }

      .start-button:hover {
        background: linear-gradient(#45a049, #3d8b40);
      }

      .progress-container {
        position: absolute;
        bottom: 50px;
        left: 50px;
        width: 500px;
      }

      .progress-bar {
        height: 20px;
        background: #333;
        border-radius: 10px;
        overflow: hidden;
      }

      .progress-fill {
        height: 100%;
        width: 0%;
        background: linear-gradient(90deg, #4caf50, #8bc34a);
        transition: width 0.3s;
      }

      .status-text {
        color: white;
        margin-top: 10px;
        text-shadow: 1px 1px 2px black;
      }
    </style>
  </head>
  <body>
    <button class="start-button" onclick="window.external.Start()">
      INICIAR JOGO
    </button>

    <button class="close-button" onclick="window.external.Close()">X</button>

    <div class="progress-container">
      <div class="progress-bar">
        <div class="progress-fill" id="progress"></div>
      </div>
      <div class="status-text" id="status">Pronto para atualizar</div>
    </div>

    <script>
      // O patcher chama essas funções automaticamente
      function setProgress(percent) {
        document.getElementById("progress").style.width = percent + "%";
      }

      function setStatus(message) {
        document.getElementById("status").textContent = message;
      }

      function onPatchComplete() {
        setStatus("Atualização concluída!");
        document.querySelector(".start-button").disabled = false;
      }

      function onPatchError(error) {
        setStatus("Erro: " + error);
      }
    </script>
  </body>
</html>
```

#### API JavaScript Disponível

O patcher expõe os seguintes métodos via `window.external`:

| Método                          | Descrição                     |
| ------------------------------- | ----------------------------- |
| `window.external.Start()`       | Inicia o jogo                 |
| `window.external.Close()`       | Fecha o patcher               |
| `window.external.Minimize()`    | Minimiza a janela             |
| `window.external.CheckUpdate()` | Verifica e baixa atualizações |
| `window.external.OpenURL(url)`  | Abre URL no navegador         |

#### Callbacks Automáticos

O patcher chama automaticamente estas funções se existirem:

| Função                 | Parâmetros | Descrição                   |
| ---------------------- | ---------- | --------------------------- |
| `setProgress(percent)` | 0-100      | Atualiza progresso          |
| `setStatus(message)`   | string     | Atualiza mensagem de status |
| `setFileName(name)`    | string     | Nome do arquivo atual       |
| `setSpeed(speed)`      | string     | Velocidade de download      |
| `onPatchComplete()`    | -          | Chamado ao concluir         |
| `onPatchError(error)`  | string     | Chamado em caso de erro     |
| `onPatchStart()`       | -          | Chamado ao iniciar          |

---

## 🌐 Configuração do Servidor

### Estrutura de Arquivos no Servidor

```
seu-servidor.com/
├── patches/
│   ├── patch001.thor
│   ├── patch002.thor
│   ├── patch003.thor
│   └── ...
├── patchlist.txt
└── version.json (opcional)
```

### Configurando o Servidor Web

#### Nginx

```nginx
server {
    listen 80;
    server_name seuservidor.com;

    location /patches/ {
        alias /var/www/patches/;
        autoindex on;

        # Headers para download
        add_header Content-Disposition "attachment";
        add_header Accept-Ranges bytes;
    }

    location /patchlist.txt {
        alias /var/www/patchlist.txt;
        add_header Cache-Control "no-cache, no-store, must-revalidate";
    }
}
```

#### Apache

```apache
<VirtualHost *:80>
    ServerName seuservidor.com
    DocumentRoot /var/www/

    <Directory /var/www/patches>
        Options Indexes
        AllowOverride None
        Require all granted

        # Força download
        Header set Content-Disposition "attachment"
    </Directory>

    # Desabilita cache do patchlist
    <Files "patchlist.txt">
        Header set Cache-Control "no-cache, no-store, must-revalidate"
    </Files>
</VirtualHost>
```

#### IIS (Windows)

1. Abra o **Gerenciador do IIS**
2. Crie um novo site ou use um existente
3. Adicione tipos MIME:
   - `.thor` → `application/octet-stream`
   - `.grf` → `application/octet-stream`
4. Configure headers HTTP personalizados para a pasta de patches

---

## 📄 Formato dos Arquivos

### patchlist.txt

O arquivo `patchlist.txt` contém a lista de todos os patches disponíveis.

#### Formato:

```
# Comentários começam com #
# ID ARQUIVO [HASH]

1 patch001.thor
2 patch002.thor
3 patch003.thor 5d41402abc4b2a76b9719d911017c592
4 patch004.thor
```

#### Regras:

- Cada linha é um patch
- Linhas começando com `#` são ignoradas
- Linhas em branco são ignoradas
- O ID deve ser único e sequencial
- O hash MD5 é opcional mas recomendado

### version.json (Opcional)

Permite verificar a versão do patcher e forçar atualizações.

```json
{
  "version": "1.0.5",
  "minVersion": "1.0.0",
  "downloadUrl": "https://seuservidor.com/patcher/AutoPatcher.exe",
  "changelog": [
    "1.0.5 - Correção de bugs",
    "1.0.4 - Novo sistema de interface",
    "1.0.3 - Melhorias de performance"
  ],
  "news": [
    {
      "title": "Evento de Natal!",
      "date": "2024-12-01",
      "url": "https://seuservidor.com/news/natal"
    }
  ]
}
```

### Arquivos THOR

Os arquivos THOR são o formato recomendado para patches.

#### Estrutura:

```
Magic: "ASSF (C) 2007 Aeomin DEV"
Versão: 2 bytes
Modo: 1 byte (0x30 = GRF, 0x31 = Pasta)
FileCount: 4 bytes
FileTable: [
    {
        PathLength: 2 bytes
        Path: string
        Flags: 1 byte
        Offset: 4 bytes
        CompressedSize: 4 bytes
        OriginalSize: 4 bytes
    }
    ...
]
Data: [bytes comprimidos]
```

#### Criando Arquivos THOR

Use o **Thor Patcher Tools** ou crie via código:

```cpp
// Exemplo simplificado
ThorArchive archive;
archive.create("patch001.thor");
archive.addFile("data/sprite/monster.spr", THOR_MODE_GRF);
archive.addFile("data/texture/item.bmp", THOR_MODE_GRF);
archive.removeFile("data/old_file.txt"); // Flag de remoção
archive.save();
```

---

## 🎮 Ações e Botões

### Lista de Ações Disponíveis

| Ação              | Descrição                       | Uso                |
| ----------------- | ------------------------------- | ------------------ |
| `start_game`      | Inicia o executável configurado | Botão principal    |
| `check_files`     | Verifica e baixa patches        | Botão de atualizar |
| `close`           | Fecha o patcher                 | Botão X            |
| `minimize`        | Minimiza a janela               | Botão -            |
| `settings`        | Abre configurações (futuro)     | Botão de config    |
| `url:https://...` | Abre URL no navegador           | Links externos     |
| `register`        | Abre página de registro         | Link de registro   |
| `website`         | Abre site principal             | Link do site       |

### Exemplos de Configuração

```json
{
  "buttons": [
    {
      "id": "start",
      "x": 200,
      "y": 300,
      "width": 120,
      "height": 40,
      "text": "JOGAR",
      "action": "start_game"
    },
    {
      "id": "update",
      "x": 200,
      "y": 350,
      "width": 120,
      "height": 40,
      "text": "ATUALIZAR",
      "action": "check_files"
    },
    {
      "id": "website",
      "x": 50,
      "y": 400,
      "width": 100,
      "height": 30,
      "text": "SITE",
      "action": "url:https://seuservidor.com"
    },
    {
      "id": "discord",
      "x": 160,
      "y": 400,
      "width": 100,
      "height": 30,
      "text": "DISCORD",
      "action": "url:https://discord.gg/seuservidor"
    }
  ]
}
```

---

## 🔍 Solução de Problemas

### Problema: "Falha ao conectar ao servidor"

**Causas possíveis:**

1. URL do servidor incorreta
2. Servidor offline
3. Firewall bloqueando conexão
4. SSL/HTTPS não configurado corretamente

**Soluções:**

1. Verifique a URL no Builder
2. Teste a URL no navegador
3. Adicione exceção no firewall
4. Use HTTP em vez de HTTPS para testes

### Problema: "Erro ao extrair arquivo"

**Causas possíveis:**

1. Arquivo THOR corrompido
2. Espaço em disco insuficiente
3. GRF em uso por outro programa
4. Permissões de escrita negadas

**Soluções:**

1. Recrie o arquivo THOR
2. Libere espaço em disco
3. Feche o cliente do jogo
4. Execute como Administrador

### Problema: "Patcher não inicia"

**Causas possíveis:**

1. Antivírus bloqueando
2. Falta de runtime VC++
3. Configuração corrompida

**Soluções:**

1. Adicione exceção no antivírus
2. Instale Visual C++ Redistributable
3. Recompile o patcher

### Problema: "Download muito lento"

**Causas possíveis:**

1. Servidor com pouca banda
2. CDN não configurada
3. Conexão instável

**Soluções:**

1. Use uma CDN (Cloudflare, AWS CloudFront)
2. Compacte melhor os arquivos THOR
3. Use servidores mais próximos geograficamente

### Problema: "Hash não confere"

**Causas possíveis:**

1. Download incompleto
2. Arquivo modificado no servidor
3. Erro de transferência

**Soluções:**

1. Tente baixar novamente
2. Verifique o arquivo original no servidor
3. Verifique a integridade da conexão

---

## ❓ FAQ

### Perguntas Gerais

**P: O patcher funciona em Linux/Mac?**
R: Não nativamente. O patcher é desenvolvido para Windows. Para outros sistemas, seria necessário usar Wine ou criar uma versão específica.

**P: Posso usar qualquer imagem como background?**
R: Sim, desde que seja PNG, JPG ou BMP. Recomendamos PNG para melhor qualidade e suporte a transparência.

**P: O patcher precisa de internet para funcionar?**
R: Sim, para verificar e baixar atualizações. Porém, se não houver atualizações pendentes, o jogo pode ser iniciado offline.

### Perguntas Técnicas

**P: Qual o tamanho máximo de um arquivo THOR?**
R: Não há limite técnico, mas recomendamos patches de até 100MB para melhor experiência do usuário.

**P: Posso usar HTTPS?**
R: Sim, o patcher suporta HTTPS. Certifique-se de que o certificado SSL seja válido.

**P: Como debug erros no patcher?**
R: Verifique os logs em `%APPDATA%/AutoPatch/logs/` ou execute via linha de comando para ver saída de debug.

**P: Posso personalizar as mensagens de erro?**
R: Sim, no modo HTML você tem controle total. No modo imagem, as mensagens são padrão.

### Perguntas sobre Servidor

**P: Preciso de um servidor dedicado?**
R: Não necessariamente. Qualquer hospedagem que sirva arquivos estáticos funciona (incluindo GitHub Pages, AWS S3, etc.).

**P: Quanto de banda preciso?**
R: Depende do número de jogadores e tamanho dos patches. Como referência: 100 jogadores × 50MB de patch = 5GB de transferência por atualização.

**P: Posso usar CDN?**
R: Sim e é recomendado! Use Cloudflare (gratuito) ou outra CDN para melhor performance global.

---

## 📞 Suporte

Para suporte adicional:

- **Email:** saadrcaa@gmail.com
- **GitHub:** Abra uma issue no repositório

---

## 📜 Licença

MIT License - Uso livre para projetos pessoais e comerciais.

---

**AutoPatch Community** - Desenvolvido com ❤️ por Cremané e a comunidade.
