import { defineStore } from 'pinia'
import { ref } from 'vue'

export type TabType = 'design' | 'html' | 'css' | 'js' | 'settings'

export const useUiStore = defineStore('ui', () => {
  // State
  const activeTab = ref<TabType>('design')
  const isSidebarOpen = ref(true)
  const isPropertiesPanelOpen = ref(true)
  const isLayersPanelOpen = ref(false)
  const statusMessage = ref('')
  const isBuilding = ref(false)
  const buildProgress = ref(0)

  // Canvas state
  const canvasZoom = ref(1)
  const canvasOffsetX = ref(0)
  const canvasOffsetY = ref(0)
  const showGrid = ref(true)
  const snapToGrid = ref(true)
  const gridSize = ref(10)
  const isProgressBarSelected = ref(false)
  const isVideoButtonSelected = ref(false)
  const isBackgroundSelected = ref(false)

  // Refresh counter (incrementing forces reactivity)
  const refreshCounter = ref(0)

  // Actions
  function setActiveTab(tab: TabType) {
    activeTab.value = tab
  }

  function toggleSidebar() {
    isSidebarOpen.value = !isSidebarOpen.value
  }

  function togglePropertiesPanel() {
    isPropertiesPanelOpen.value = !isPropertiesPanelOpen.value
  }

  function toggleLayersPanel() {
    isLayersPanelOpen.value = !isLayersPanelOpen.value
  }

  function setStatus(message: string) {
    statusMessage.value = message
  }

  function clearStatus() {
    statusMessage.value = ''
  }

  function startBuild() {
    isBuilding.value = true
    buildProgress.value = 0
  }

  function updateBuildProgress(progress: number, message?: string) {
    buildProgress.value = progress
    if (message) {
      statusMessage.value = message
    }
  }

  function endBuild() {
    isBuilding.value = false
    buildProgress.value = 100
  }

  function setCanvasZoom(zoom: number) {
    canvasZoom.value = Math.max(0.25, Math.min(2, zoom))
  }

  function zoomIn() {
    setCanvasZoom(canvasZoom.value + 0.1)
  }

  function zoomOut() {
    setCanvasZoom(canvasZoom.value - 0.1)
  }

  function resetZoom() {
    canvasZoom.value = 1
    canvasOffsetX.value = 0
    canvasOffsetY.value = 0
  }

  function toggleGrid() {
    showGrid.value = !showGrid.value
  }

  function toggleSnap() {
    snapToGrid.value = !snapToGrid.value
  }

  function setGridSize(size: number) {
    gridSize.value = Math.max(5, Math.min(50, size))
  }

  function selectProgressBar(selected: boolean) {
    isProgressBarSelected.value = selected
    if (selected) {
      isVideoButtonSelected.value = false
      isBackgroundSelected.value = false
    }
  }

  function selectVideoButton(selected: boolean) {
    isVideoButtonSelected.value = selected
    if (selected) {
      isProgressBarSelected.value = false
      isBackgroundSelected.value = false
    }
  }

  function selectBackground(selected: boolean) {
    isBackgroundSelected.value = selected
    if (selected) {
      isProgressBarSelected.value = false
      isVideoButtonSelected.value = false
    }
  }

  function refreshAll() {
    refreshCounter.value++
    setStatus('Recarregando...')
    setTimeout(() => setStatus('Recarregado com sucesso!'), 500)
  }

  return {
    // State
    activeTab,
    isSidebarOpen,
    isPropertiesPanelOpen,
    isLayersPanelOpen,
    statusMessage,
    isBuilding,
    buildProgress,
    canvasZoom,
    canvasOffsetX,
    canvasOffsetY,
    showGrid,
    snapToGrid,
    gridSize,
    isProgressBarSelected,
    isVideoButtonSelected,
    isBackgroundSelected,
    refreshCounter,
    // Actions
    setActiveTab,
    toggleSidebar,
    togglePropertiesPanel,
    toggleLayersPanel,
    setStatus,
    clearStatus,
    startBuild,
    updateBuildProgress,
    endBuild,
    setCanvasZoom,
    zoomIn,
    zoomOut,
    resetZoom,
    toggleGrid,
    toggleSnap,
    setGridSize,
    selectProgressBar,
    selectVideoButton,
    selectBackground,
    refreshAll
  }
})
