<script setup lang="ts">
import { computed } from 'vue'
import { useProjectStore } from './stores/project'
import { useUiStore } from './stores/ui'
import Sidebar from './components/layout/Sidebar.vue'
import StatusBar from './components/layout/StatusBar.vue'
import DesignCanvas from './components/editor/DesignCanvas.vue'
import PropertyPanel from './components/editor/PropertyPanel.vue'
import CodeEditor from './components/editor/CodeEditor.vue'
import SettingsPanel from './components/editor/SettingsPanel.vue'

const projectStore = useProjectStore()
const uiStore = useUiStore()

// Show design view when in image mode
const showDesignView = computed(() => 
  projectStore.project.config.uiMode === 'image' && uiStore.activeTab === 'design'
)

// Show code editor when in HTML mode
const showCodeView = computed(() => 
  projectStore.project.config.uiMode === 'html' && ['html', 'css', 'js'].includes(uiStore.activeTab)
)

const showSettingsView = computed(() => uiStore.activeTab === 'settings')
</script>

<template>
  <div class="app-container">
    <!-- Sidebar -->
    <Sidebar />

    <!-- Main Content -->
    <div class="main-content">
      <!-- Editor Area -->
      <div class="editor-area">
        <!-- Design Canvas (Image Mode) -->
        <DesignCanvas v-if="showDesignView" />

        <!-- Code Editor (HTML Mode) -->
        <CodeEditor v-if="showCodeView" />

        <!-- Settings Panel -->
        <SettingsPanel v-if="showSettingsView" />

        <!-- Properties Panel (only in design mode) -->
        <PropertyPanel
          v-if="showDesignView && uiStore.isPropertiesPanelOpen"
        />
      </div>

      <!-- Status Bar -->
      <StatusBar />
    </div>
  </div>
</template>

<style scoped>
.app-container {
  display: flex;
  height: 100vh;
  width: 100vw;
  overflow: hidden;
  background-color: #1e1e1e;
}

.main-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
}

.editor-area {
  flex: 1;
  display: flex;
  overflow: hidden;
  background-color: #252526;
}
</style>
