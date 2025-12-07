#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace autopatch
{

    // Tipo de UI do patcher
    enum class UIType
    {
        Image = 0, // Background + botões posicionados
        Html = 1   // Interface HTML/CSS/JS
    };

    // Alinhamento de texto horizontal
    enum class TextAlignment
    {
        Left = 0,
        Center = 1,
        Right = 2
    };

    // Alinhamento de texto vertical
    enum class VerticalAlignment
    {
        Top = 0,
        Middle = 1,
        Bottom = 2
    };

    // Tipo de elemento UI
    enum class ElementType
    {
        Button,
        Label,
        Status,
        Percentage,
        Box,
        Image
    };

    // Estado visual de um elemento
    struct ElementState
    {
        std::string imagePath;       // Imagem para este estado
        std::string backgroundColor; // Cor de fundo
        std::string fontColor;       // Cor do texto
        std::string borderColor;     // Cor da borda
        int opacity = 100;           // Opacidade (0-100)
        int offsetX = 0;             // Deslocamento X (para pressed)
        int offsetY = 0;             // Deslocamento Y (para pressed)
    };

    // Efeito de sombra
    struct ShadowEffect
    {
        bool enabled = false;
        std::string color = "#000000";
        int blur = 10;
        int offsetX = 0;
        int offsetY = 4;
    };

    // Efeito de brilho
    struct GlowEffect
    {
        bool enabled = false;
        std::string color = "#00ff00";
        int intensity = 10;
    };

    // Efeitos visuais
    struct ElementEffects
    {
        int opacity = 100;    // Opacidade geral (0-100)
        int borderRadius = 0; // Arredondamento de bordas
        int rotation = 0;     // Rotação em graus
        ShadowEffect shadow;
        GlowEffect glow;
    };

    // Estilo de box translúcida
    struct BoxStyle
    {
        std::string fillColor = "#000000";
        int fillOpacity = 50; // 0-100
        std::string borderColor = "#ffffff";
        int borderWidth = 1;
        int borderRadius = 8;
    };

    // Configuração de um botão
    struct ButtonConfig
    {
        std::string id;
        std::string name;
        std::string action; // "start_game", "check_files", "settings", "close", "minimize", "website"
        int x = 0;
        int y = 0;
        int width = 100;
        int height = 30;
        std::string text;
        std::string tooltip;

        // Fonte
        std::string fontName = "Segoe UI";
        int fontSize = 14;
        std::string fontColor = "#FFFFFF";
        bool fontBold = true;
        bool fontItalic = false;
        TextAlignment textAlign = TextAlignment::Center;
        VerticalAlignment textVerticalAlign = VerticalAlignment::Middle;

        // Aparência base
        std::string backgroundColor = "#0078d4";
        std::string backgroundImage;
        std::string borderColor = "#005a9e";
        int borderWidth = 1;

        // Estados do botão
        std::optional<ElementState> normalState;
        std::optional<ElementState> hoverState;
        std::optional<ElementState> pressedState;
        std::optional<ElementState> disabledState;

        // Imagens legadas (para compatibilidade)
        std::string normalImage;
        std::string hoverImage;
        std::string pressedImage;
        std::string disabledImage;

        // Efeitos
        ElementEffects effects;

        // Controle
        bool visible = true;
        bool locked = false;
        int zIndex = 10;
    };

    // Configuração de um label
    struct LabelConfig
    {
        std::string id;
        std::string name;
        ElementType type = ElementType::Label; // Label, Status ou Percentage
        int x = 0;
        int y = 0;
        int width = 200;
        int height = 20;
        std::string text;

        // Fonte
        std::string fontName = "Segoe UI";
        int fontSize = 12;
        std::string fontColor = "#FFFFFF";
        bool fontBold = false;
        bool fontItalic = false;
        TextAlignment textAlign = TextAlignment::Left;
        VerticalAlignment textVerticalAlign = VerticalAlignment::Middle;

        // Efeitos
        ElementEffects effects;

        // Flags especiais (legado)
        bool isStatusLabel = false;     // Se true, mostra o status do patch
        bool isPercentageLabel = false; // Se true, mostra a porcentagem

        // Controle
        bool visible = true;
        bool locked = false;
        int zIndex = 5;
    };

    // Configuração de uma box translúcida
    struct BoxConfig
    {
        std::string id;
        std::string name;
        int x = 0;
        int y = 0;
        int width = 200;
        int height = 150;

        BoxStyle style;
        ElementEffects effects;

        bool visible = true;
        bool locked = false;
        int zIndex = 1;
    };

    // Configuração de uma imagem decorativa
    struct ImageConfig
    {
        std::string id;
        std::string name;
        int x = 0;
        int y = 0;
        int width = 100;
        int height = 100;

        std::string imagePath;
        ElementEffects effects;

        bool visible = true;
        bool locked = false;
        int zIndex = 2;
    };

    // Configuração de um WebView (iframe para conteúdo externo)
    struct WebViewConfig
    {
        std::string id;
        std::string name;
        int x = 0;
        int y = 0;
        int width = 300;
        int height = 200;

        std::string url;      // URL da página a ser exibida
        int borderRadius = 8; // Arredondamento das bordas
        std::string borderColor = "#333333";
        int borderWidth = 1;
        std::string backgroundColor = "#1e1e1e";

        ElementEffects effects;

        bool visible = true;
        bool locked = false;
        int zIndex = 3;
    };

    // Configuração da barra de progresso
    struct ProgressBarConfig
    {
        int x = 0;
        int y = 0;
        int width = 400;
        int height = 20;
        std::string backgroundColor = "#333333";
        std::string fillColor = "#00FF00";
        std::string borderColor = "#666666";
        int borderRadius = 0;
        std::string backgroundImage;
        std::string fillImage;
    };

    // Estilo do botão de controle de vídeo
    struct VideoControlButtonStyle
    {
        int x = 740;   // Posição X
        int y = 550;   // Posição Y
        int size = 50; // Tamanho (diâmetro)
        std::string backgroundColor = "#000000";
        std::string iconColor = "#ffffff";
        std::string borderColor = "#ffffff";
        int borderWidth = 2;
        int opacity = 50; // 0-100
    };

    // Configuração de vídeo de fundo
    struct VideoBackgroundConfig
    {
        bool enabled = false;
        std::string videoFile;                 // Nome do arquivo de vídeo (ex: "background.mp4")
        bool loop = true;                      // Repetir o vídeo
        bool autoplay = true;                  // Iniciar automaticamente
        bool muted = true;                     // Sem som
        bool showControls = false;             // Mostrar botão play/pause
        VideoControlButtonStyle controlButton; // Estilo do botão
    };

    // Configuração do modo imagem
    struct ImageModeConfig
    {
        std::vector<ButtonConfig> buttons;
        std::vector<LabelConfig> labels;
        std::vector<BoxConfig> boxes;
        std::vector<ImageConfig> images;
        std::vector<WebViewConfig> webviews;
        ProgressBarConfig progressBar;
        std::string backgroundImage; // Base64 encoded
        std::string fontPath;
        VideoBackgroundConfig videoBackground; // Vídeo de fundo
    };

    // Configuração do modo HTML
    struct HtmlModeConfig
    {
        std::string startButtonId = "btn-start";
        std::string progressBarId = "progress-bar";
        std::string statusLabelId = "status-text";
        std::string closeButtonId = "btn-close";
        std::string minimizeButtonId = "btn-minimize";
    };

    // Configuração principal do patcher
    struct PatcherConfig
    {
        // Informações do servidor
        std::string serverName;
        std::string patchListUrl;
        std::string newsUrl;

        // Executável do jogo
        std::string clientExe = "ragexe.exe";
        std::string clientArgs;

        // GRFs para patch
        std::vector<std::string> grfFiles;

        // UI
        UIType uiType = UIType::Image;
        int windowWidth = 800;
        int windowHeight = 600;
        int windowBorderRadius = 0; // Arredondamento das bordas da janela
        bool allowResize = false;
        bool showInTaskbar = true;

        // Configurações de modo (usando shared_ptr para permitir cópia)
        std::shared_ptr<ImageModeConfig> imageMode;
        std::shared_ptr<HtmlModeConfig> htmlMode;

        // Opções avançadas
        bool allowMultipleInstances = false;
        bool checkForPatcherUpdates = true;
        std::string patcherUpdateUrl;
    };

    // Função para carregar a configuração (do EXE embutido ou arquivo externo)
    PatcherConfig LoadConfig();

    // Extrai configuração embutida no EXE
    std::string ExtractEmbeddedConfig();

} // namespace autopatch
