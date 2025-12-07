import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type {
  Project,
  UIElement,
  ElementType
} from '../types'
import {
  createDefaultProject as createProject,
  createDefaultButton as createButton,
  createDefaultLabel as createLabel,
  createDefaultStatusLabel as createStatus,
  createDefaultPercentageLabel as createPercentage,
  createDefaultBox as createBox,
  createDefaultImage as createImage,
  createDefaultWebview as createWebview
} from '../types'

export const useProjectStore = defineStore('project', () => {
  // State
  const project = ref<Project>(createProject())
  const selectedElementId = ref<string | null>(null)

  // Getters
  const selectedElement = computed(() => {
    if (!selectedElementId.value) return null
    return project.value.config.elements.find((e) => e.id === selectedElementId.value) || null
  })

  const hasUnsavedChanges = computed(() => project.value.isDirty)

  // Actions
  function newProject() {
    project.value = createProject()
    selectedElementId.value = null
  }

  function loadProject(data: Project) {
    // Ensure videoBackground has all required fields with defaults
    if (!data.config.videoBackground) {
      data.config.videoBackground = {
        enabled: false,
        path: '',
        loop: true,
        autoplay: true,
        muted: true,
        showControls: true,
        controlButton: {
          x: 720,
          y: 440,
          size: 50,
          backgroundColor: '#333333',
          iconColor: '#ffffff',
          borderColor: '#666666',
          borderWidth: 2,
          opacity: 100
        }
      }
    } else if (!data.config.videoBackground.controlButton) {
      // Add controlButton if missing
      data.config.videoBackground.controlButton = {
        x: 720,
        y: 440,
        size: 50,
        backgroundColor: '#333333',
        iconColor: '#ffffff',
        borderColor: '#666666',
        borderWidth: 2,
        opacity: 100
      }
    }
    
    project.value = data
    project.value.isDirty = false
    selectedElementId.value = null
  }

  function markDirty() {
    project.value.isDirty = true
  }

  function markSaved() {
    project.value.isDirty = false
  }

  function setProjectPath(path: string) {
    project.value.path = path
  }

  // Element management
  function generateId(): string {
    return `elem_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`
  }

  function addElement(type: ElementType): UIElement {
    const id = generateId()
    let element: UIElement

    switch (type) {
      case 'button':
        element = createButton(id)
        break
      case 'status':
        element = createStatus(id)
        break
      case 'percentage':
        element = createPercentage(id)
        break
      case 'box':
        element = createBox(id)
        break
      case 'image':
        element = createImage(id)
        break
      case 'webview':
        element = createWebview(id)
        break
      default:
        element = createLabel(id)
    }

    project.value.config.elements.push(element)
    selectedElementId.value = id
    markDirty()
    return element
  }

  function removeElement(id: string) {
    const index = project.value.config.elements.findIndex((e) => e.id === id)
    if (index !== -1) {
      project.value.config.elements.splice(index, 1)
      if (selectedElementId.value === id) {
        selectedElementId.value = null
      }
      markDirty()
    }
  }

  // Layer management functions
  function bringToFront(id: string) {
    const elements = project.value.config.elements
    const maxZ = Math.max(...elements.map(e => e.zIndex || 0), 0)
    const element = elements.find(e => e.id === id)
    if (element) {
      element.zIndex = maxZ + 1
      markDirty()
    }
  }

  function sendToBack(id: string) {
    const elements = project.value.config.elements
    const minZ = Math.min(...elements.map(e => e.zIndex || 0), 0)
    const element = elements.find(e => e.id === id)
    if (element) {
      element.zIndex = minZ - 1
      markDirty()
    }
  }

  function moveLayerUp(id: string) {
    const elements = project.value.config.elements
    const element = elements.find(e => e.id === id)
    if (!element) return

    const currentZ = element.zIndex || 0
    // Find the element with the smallest zIndex greater than current
    const elementsAbove = elements.filter(e => (e.zIndex || 0) > currentZ)
    if (elementsAbove.length === 0) {
      // Already at top, just increment
      element.zIndex = currentZ + 1
    } else {
      // Find the closest one above
      const closest = elementsAbove.reduce((prev, curr) => 
        (curr.zIndex || 0) < (prev.zIndex || 0) ? curr : prev
      )
      // Swap z-indices
      const closestZ = closest.zIndex || 0
      closest.zIndex = currentZ
      element.zIndex = closestZ
    }
    markDirty()
  }

  function moveLayerDown(id: string) {
    const elements = project.value.config.elements
    const element = elements.find(e => e.id === id)
    if (!element) return

    const currentZ = element.zIndex || 0
    // Find the element with the largest zIndex less than current
    const elementsBelow = elements.filter(e => (e.zIndex || 0) < currentZ)
    if (elementsBelow.length === 0) {
      // Already at bottom, just decrement
      element.zIndex = currentZ - 1
    } else {
      // Find the closest one below
      const closest = elementsBelow.reduce((prev, curr) => 
        (curr.zIndex || 0) > (prev.zIndex || 0) ? curr : prev
      )
      // Swap z-indices
      const closestZ = closest.zIndex || 0
      closest.zIndex = currentZ
      element.zIndex = closestZ
    }
    markDirty()
  }

  // Get elements sorted by z-index for layer panel
  function getElementsSortedByLayer() {
    return [...project.value.config.elements].sort((a, b) => (b.zIndex || 0) - (a.zIndex || 0))
  }

  function selectElement(id: string | null) {
    selectedElementId.value = id
  }

  function updateElement(id: string, updates: Partial<UIElement>) {
    const element = project.value.config.elements.find((e) => e.id === id)
    if (element) {
      Object.assign(element, updates)
      markDirty()
    }
  }

  function moveElement(id: string, x: number, y: number) {
    updateElement(id, { x, y })
  }

  function resizeElement(id: string, width: number, height: number) {
    updateElement(id, { width, height })
  }

  // Config management
  function updateConfig(updates: Partial<Project['config']>) {
    Object.assign(project.value.config, updates)
    markDirty()
  }

  function setBackgroundImage(path: string) {
    project.value.config.backgroundImagePath = path
    markDirty()
  }

  function setWindowSize(width: number, height: number) {
    project.value.config.windowWidth = width
    project.value.config.windowHeight = height
    markDirty()
  }

  function setWindowBorderRadius(radius: number) {
    project.value.config.windowBorderRadius = radius
    markDirty()
  }

  function setUiMode(mode: 'image' | 'html') {
    project.value.config.uiMode = mode
    markDirty()
  }

  function updateProgressBar(updates: Partial<Project['config']['progressBar']>) {
    Object.assign(project.value.config.progressBar, updates)
    markDirty()
  }

  // Serialization
  function toJSON(): string {
    return JSON.stringify(project.value, null, 2)
  }

  function fromJSON(json: string) {
    try {
      const data = JSON.parse(json) as Project
      loadProject(data)
    } catch (e) {
      console.error('Failed to parse project JSON:', e)
      throw new Error('Arquivo de projeto inválido')
    }
  }

  return {
    // State
    project,
    selectedElementId,
    // Getters
    selectedElement,
    hasUnsavedChanges,
    // Actions
    newProject,
    loadProject,
    markDirty,
    markSaved,
    setProjectPath,
    addElement,
    removeElement,
    selectElement,
    updateElement,
    moveElement,
    resizeElement,
    updateConfig,
    setBackgroundImage,
    setWindowSize,
    setWindowBorderRadius,
    setUiMode,
    updateProgressBar,
    bringToFront,
    sendToBack,
    moveLayerUp,
    moveLayerDown,
    getElementsSortedByLayer,
    toJSON,
    fromJSON
  }
})
