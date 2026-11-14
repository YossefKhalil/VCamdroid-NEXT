package com.darusc.vcamdroid.video.filters

import com.pedro.encoder.input.gl.render.filters.*

private typealias Adjuster = (BaseFilterRender, Float) -> Unit

object FilterAdjuster {

    /**
     * @param filter The base filter to adjust
     * @param value The adjustment 0-100
     */
    fun adjust(filter: BaseFilterRender, value: Int) {
        val normalized = value / 100f
        val adjuster = registry[filter::class.java]

        adjuster?.invoke(filter, normalized)
    }

    private val registry = mapOf<Class<out BaseFilterRender>, Adjuster>(

        BrightnessFilterRender::class.java to { f, p ->
            (f as BrightnessFilterRender).brightness = (p * 2) - 1
        },

        ContrastFilterRender::class.java to { f, p ->
            (f as ContrastFilterRender).contrast = p * 2
        },

        ExposureFilterRender::class.java to { f, p ->
            (f as ExposureFilterRender).exposure = (p * 2) - 1
        },

        GammaFilterRender::class.java to { f, p ->
            (f as GammaFilterRender).gamma = p * 2
        },

        SaturationFilterRender::class.java to { f, p ->
            // setSaturation expects -1..1 (0 = unchanged). Mapping 0..100
            // directly to 0..2 made the default value of 50 an extreme boost.
            (f as SaturationFilterRender).saturation = (p * 2) - 1
        },

        TemperatureFilterRender::class.java to { f, p ->
            (f as TemperatureFilterRender).setTemperature(p)
        },

        SharpnessFilterRender::class.java to { f, p ->
            // 0 = no change in RootEncoder, so the neutral slider position (50)
            // must map to 0 instead of a constant strong sharpening pass.
            (f as SharpnessFilterRender).sharpness = ((p - 0.5f) * 2f).coerceIn(0f, 1f)
        },
    )
}