/* Copyright 2011-2016 CoReNum, INSA-Lyon, ENS-Lyon
 *
 * This file is part of libcrn.
 *
 * libcrn is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libcrn is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libcrn.  If not, see <http://www.gnu.org/licenses/>.
 *
 * file: titus.cpp
 * \author Yann LEYDIER
 */

#define GETTEXT_PACKAGE "titus"
#include <CRN.h>
#include <GtkCRNMain.h>

#include <GtkCRNApp.h>
#include <GdkCRNPixbuf.h>
#include <CRNImage/CRNImageRGB.h>
#include <CRNImage/CRNImageHSV.h>
#include <CRNImage/CRNImageGray.h>
#include <CRNImage/CRNImageBW.h>
#include <CRNImage/CRNImageAngle.h>
#include <CRNImage/CRNDifferential.h>
#include <CRNMath/CRNMatrixDouble.h>
#include <GtkCRNImage.h>
#include <CRNIO/CRNIO.h>
#include <CRNConfig.h>
#include <GtkCRNHelpers.h>
#include <CRNi18n.h>

#include <CRNUtils/CRNTimer.h>
#include <iostream>

#ifndef CRN_USING_GTKMM3
#	define get_content_area get_vbox
#endif

using namespace crn;

class Titus: public GtkCRN::App
{
	public:
		Titus():
			current_image(NONE),
			fdial(*this, _("Please select an image"), Gtk::FILE_CHOOSER_ACTION_OPEN),
			refreshing(false)
#ifdef CRN_USING_GTKMM3
			,
			ff(Gtk::FileFilter::create())
#endif
		{
			Glib::ustring title("Titus ");
			title += CRN_PACKAGE_VERSION;
			title += " © CoReNum";
			set_title(title);

#ifdef CRN_USING_GTKMM3
			// file menu
			actions->add_action("open-image", sigc::mem_fun(this, &Titus::open_image));
			actions->add_action("save-image", sigc::mem_fun(this, &Titus::save_image));
			// toolbar
			actions->add_action_radio_integer("show-image", sigc::mem_fun(this, &Titus::on_image_toggled), 0); // XXX TODO Crash on windows

			// generic menu
			actions->add_action("generic-menu");
			actions->add_action("generic-blur", sigc::mem_fun(this, &Titus::generic_blur));
			actions->add_action("generic-blur-x", sigc::mem_fun(this, &Titus::generic_blur_x));
			actions->add_action("generic-blur-y", sigc::mem_fun(this, &Titus::generic_blur_y));
			actions->add_action("generic-deriv-x", sigc::mem_fun(this, &Titus::generic_deriv_x));
			actions->add_action("generic-deriv-y", sigc::mem_fun(this, &Titus::generic_deriv_y));
			actions->add_action("generic-2deriv-x", sigc::mem_fun(this, &Titus::generic_2deriv_x));
			actions->add_action("generic-2deriv-y", sigc::mem_fun(this, &Titus::generic_2deriv_y));

			// rgb menu
			actions->add_action("rgb-menu");
			actions->add_action("rgb-red", sigc::mem_fun(this, &Titus::rgb_red));
			actions->add_action("rgb-green", sigc::mem_fun(this, &Titus::rgb_green));
			actions->add_action("rgb-blue", sigc::mem_fun(this, &Titus::rgb_blue));
			actions->add_action("rgb-hue", sigc::mem_fun(this, &Titus::rgb_hue));
			actions->add_action("rgb-saturation", sigc::mem_fun(this, &Titus::rgb_saturation));
			actions->add_action("rgb-pseudosaturation", sigc::mem_fun(this, &Titus::rgb_pseudosaturation));
			actions->add_action("rgb-value", sigc::mem_fun(this, &Titus::rgb_value));
			actions->add_action("rgb-y", sigc::mem_fun(this, &Titus::rgb_y));
			actions->add_action("rgb-l", sigc::mem_fun(this, &Titus::rgb_l));
			actions->add_action("rgb-lprime", sigc::mem_fun(this, &Titus::rgb_lprime));

			actions->add_action("rgb-saturate", sigc::mem_fun(this, &Titus::rgb_saturate));
			actions->add_action("rgb-edge-preserving-filter", sigc::mem_fun(this, &Titus::rgb_epf));

			actions->add_action("rgb-diff", sigc::mem_fun(this, &Titus::rgb_diff));

			// gray menu
			actions->add_action("gray-menu");
			actions->add_action("gray-to-rgb", sigc::mem_fun(this, &Titus::gray_to_rgb));
			actions->add_action("gray-threshold", sigc::mem_fun(this, &Titus::gray_threshold));
			actions->add_action("gray-fisher", sigc::mem_fun(this, &Titus::gray_fisher));
			actions->add_action("gray-entropy", sigc::mem_fun(this, &Titus::gray_entropy));
			actions->add_action("gray-otsu", sigc::mem_fun(this, &Titus::gray_otsu));
			actions->add_action("gray-niblack", sigc::mem_fun(this, &Titus::gray_niblack));
			actions->add_action("gray-sauvola", sigc::mem_fun(this, &Titus::gray_sauvola));
			actions->add_action("gray-kmh", sigc::mem_fun(this, &Titus::gray_kmh));
			actions->add_action("gray-lmin", sigc::mem_fun(this, &Titus::gray_lmin));
			actions->add_action("gray-lmax", sigc::mem_fun(this, &Titus::gray_lmax));

			actions->add_action("gray-strokes", sigc::mem_fun(this, &Titus::gray_strokes));
			actions->add_action("gray-histo", sigc::mem_fun(this, &Titus::gray_histogram));
			actions->add_action("gray-rhisto", sigc::mem_fun(this, &Titus::gray_rhistogram));

			actions->add_action("gray-diff", sigc::mem_fun(this, &Titus::gray_diff));

			// bw menu
			actions->add_action("bw-menu");
			actions->add_action("bw-to-gray", sigc::mem_fun(this, &Titus::bw_to_gray));
			actions->add_action("bw-leftprof", sigc::mem_fun(this, &Titus::bw_leftprof));
			actions->add_action("bw-rightprof", sigc::mem_fun(this, &Titus::bw_rightprof));
			actions->add_action("bw-topprof", sigc::mem_fun(this, &Titus::bw_topprof));
			actions->add_action("bw-bottomprof", sigc::mem_fun(this, &Titus::bw_bottomprof));
			actions->add_action("bw-hproj", sigc::mem_fun(this, &Titus::bw_hproj));
			actions->add_action("bw-vproj", sigc::mem_fun(this, &Titus::bw_vproj));

			// differential menu
			actions->add_action("diff-menu");
			actions->add_action("diff-diffuse", sigc::mem_fun(this, &Titus::diff_diffuse));
			actions->add_action("diff-gradgray", sigc::mem_fun(this, &Titus::diff_gradgray));
			actions->add_action("diff-gradrgb", sigc::mem_fun(this, &Titus::diff_gradrgb));
			actions->add_action("diff-gradmod", sigc::mem_fun(this, &Titus::diff_gradmod));

			actions->add_action("diff-div", sigc::mem_fun(this, &Titus::diff_div));
			actions->add_action("diff-laplacian", sigc::mem_fun(this, &Titus::diff_laplacian));
			actions->add_action("diff-edge", sigc::mem_fun(this, &Titus::diff_edge));
			actions->add_action("diff-corner", sigc::mem_fun(this, &Titus::diff_corner));
			actions->add_action("diff-k1", sigc::mem_fun(this, &Titus::diff_k1));
			actions->add_action("diff-k2", sigc::mem_fun(this, &Titus::diff_k2));
			actions->add_action("diff-hcorner", sigc::mem_fun(this, &Titus::diff_hcorner));

			actions->add_action("diff-iso", sigc::mem_fun(this, &Titus::diff_iso));
			actions->add_action("diff-flow", sigc::mem_fun(this, &Titus::diff_flow));
			actions->add_action("diff-gaussc", sigc::mem_fun(this, &Titus::diff_gaussc));
			actions->add_action("diff-gradc", sigc::mem_fun(this, &Titus::diff_gradc));

			actions->add_action("diff-lx", sigc::mem_fun(this, &Titus::diff_lx));
			actions->add_action("diff-ly", sigc::mem_fun(this, &Titus::diff_ly));
			actions->add_action("diff-lxx", sigc::mem_fun(this, &Titus::diff_lxx));
			actions->add_action("diff-lxy", sigc::mem_fun(this, &Titus::diff_lxy));
			actions->add_action("diff-lyy", sigc::mem_fun(this, &Titus::diff_lyy));
			actions->add_action("diff-lw", sigc::mem_fun(this, &Titus::diff_lw));
			actions->add_action("diff-lvv", sigc::mem_fun(this, &Titus::diff_lvv));
			actions->add_action("diff-lvw", sigc::mem_fun(this, &Titus::diff_lvw));
			actions->add_action("diff-lww", sigc::mem_fun(this, &Titus::diff_lww));

			Glib::ustring ui_info =
				"<interface>"
				"	<menu id='MenuBar'>"
				"		<submenu>"
				"			<attribute name='label' translatable='yes'>_File</attribute>"
				"			<attribute name='action'>app.file-menu</attribute>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Open</attribute>"
				"					<attribute name='action'>app.open-image</attribute>"
				"					<attribute name='accel'>&lt;Primary&gt;o</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Save</attribute>"
				"					<attribute name='action'>app.save-image</attribute>"
				"					<attribute name='accel'>&lt;Primary&gt;s</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Quit</attribute>"
				"					<attribute name='action'>app.quit</attribute>"
				"					<attribute name='accel'>&lt;Primary&gt;q</attribute>"
				"				</item>"
				"			</section>"
				"		</submenu>"
				"		<submenu>"
				"			<attribute name='label' translatable='yes'>_Generic</attribute>"
				"			<attribute name='action'>app.generic-menu</attribute>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Gaussian blur</attribute>"
				"					<attribute name='action'>app.generic-blur</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Gaussian blur along _x</attribute>"
				"					<attribute name='action'>app.generic-blur-x</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Gaussian blur along _y</attribute>"
				"					<attribute name='action'>app.generic-blur-y</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Gaussian _derivative along x</attribute>"
				"					<attribute name='action'>app.generic-deriv-x</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Gaussian d_erivative along y</attribute>"
				"					<attribute name='action'>app.generic-deriv-y</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Gaussian _second derivative along x</attribute>"
				"					<attribute name='action'>app.generic-2deriv-x</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Gaussian se_cond derivative along y</attribute>"
				"					<attribute name='action'>app.generic-2deriv-y</attribute>"
				"				</item>"
				"			</section>"
				"		</submenu>"
				"		<submenu>"
				"			<attribute name='label' translatable='yes'>_RGB</attribute>"
				"			<attribute name='action'>app.rgb-menu</attribute>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Red</attribute>"
				"					<attribute name='action'>app.rgb-red</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Green</attribute>"
				"					<attribute name='action'>app.rgb-green</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Blue</attribute>"
				"					<attribute name='action'>app.rgb-blue</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Hue</attribute>"
				"					<attribute name='action'>app.rgb-hue</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Saturation</attribute>"
				"					<attribute name='action'>app.rgb-saturation</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Pseudo s_aturation</attribute>"
				"					<attribute name='action'>app.rgb-pseudosaturation</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Value</attribute>"
				"					<attribute name='action'>app.rgb-value</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Luminance (_YUV)</attribute>"
				"					<attribute name='action'>app.rgb-y</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Luminance (HS_L)</attribute>"
				"					<attribute name='action'>app.rgb-l</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Pseudo l_uminance</attribute>"
				"					<attribute name='action'>app.rgb-lprime</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Saturate</attribute>"
				"					<attribute name='action'>app.rgb-saturate</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Edge _preserving filter</attribute>"
				"					<attribute name='action'>app.rgb-edge-preserving-filter</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Differential</attribute>"
				"					<attribute name='action'>app.rgb-diff</attribute>"
				"				</item>"
				"			</section>"
				"		</submenu>"
				"		<submenu>"
				"			<attribute name='label' translatable='yes'>_Gray</attribute>"
				"			<attribute name='action'>app.gray-menu</attribute>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>To _RGB</attribute>"
				"					<attribute name='action'>app.gray-to-rgb</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Threshold</attribute>"
				"					<attribute name='action'>app.gray-threshold</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Fisher</attribute>"
				"					<attribute name='action'>app.gray-fisher</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Entropy</attribute>"
				"					<attribute name='action'>app.gray-entropy</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Otsu</attribute>"
				"					<attribute name='action'>app.gray-otsu</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Niblack</attribute>"
				"					<attribute name='action'>app.gray-niblack</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Sauvola</attribute>"
				"					<attribute name='action'>app.gray-sauvola</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_k means histogram</attribute>"
				"					<attribute name='action'>app.gray-kmh</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Local m_in</attribute>"
				"					<attribute name='action'>app.gray-lmin</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Local m_ax</attribute>"
				"					<attribute name='action'>app.gray-lmax</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Strokes statistics</attribute>"
				"					<attribute name='action'>app.gray-strokes</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Histogram</attribute>"
				"					<attribute name='action'>app.gray-histo</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Radial histogram</attribute>"
				"					<attribute name='action'>app.gray-rhisto</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Differential</attribute>"
				"					<attribute name='action'>app.gray-diff</attribute>"
				"				</item>"
				"			</section>"
				"		</submenu>"
				"		<submenu>"
				"			<attribute name='label' translatable='yes'>_BW</attribute>"
				"			<attribute name='action'>app.bw-menu</attribute>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>To _gray</attribute>"
				"					<attribute name='action'>app.bw-to-gray</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Left profile</attribute>"
				"					<attribute name='action'>app.bw-leftprof</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Right profile</attribute>"
				"					<attribute name='action'>app.bw-rightprof</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Top profile</attribute>"
				"					<attribute name='action'>app.bw-topprof</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Bottom profile</attribute>"
				"					<attribute name='action'>app.bw-bottomprof</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Horizontal projection</attribute>"
				"					<attribute name='action'>app.bw-hproj</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Vertical projection</attribute>"
				"					<attribute name='action'>app.bw-vproj</attribute>"
				"				</item>"
				"			</section>"
				"		</submenu>"
				"		<submenu>"
				"			<attribute name='label' translatable='yes'>_Differential</attribute>"
				"			<attribute name='action'>app.diff-menu</attribute>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Diffuse</attribute>"
				"					<attribute name='action'>app.diff-diffuse</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Gradient (_grayscale)</attribute>"
				"					<attribute name='action'>app.diff-gradgray</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Gradient (_RGB)</attribute>"
				"					<attribute name='action'>app.diff-gradrgb</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Gradient _module</attribute>"
				"					<attribute name='action'>app.diff-gradmod</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Divergence</attribute>"
				"					<attribute name='action'>app.diff-div</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Laplacian</attribute>"
				"					<attribute name='action'>app.diff-laplacian</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Edge</attribute>"
				"					<attribute name='action'>app.diff-edge</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Corner</attribute>"
				"					<attribute name='action'>app.diff-corner</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Kappa _1</attribute>"
				"					<attribute name='action'>app.diff-k1</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>Kappa _2</attribute>"
				"					<attribute name='action'>app.diff-k2</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Hessian corner</attribute>"
				"					<attribute name='action'>app.diff-hcorner</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Isophote curvature</attribute>"
				"					<attribute name='action'>app.diff-iso</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Flowline curvature</attribute>"
				"					<attribute name='action'>app.diff-flow</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_Gaussian curvature</attribute>"
				"					<attribute name='action'>app.diff-gaussc</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>G_radient curvature</attribute>"
				"					<attribute name='action'>app.diff-gradc</attribute>"
				"				</item>"
				"			</section>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>L_x</attribute>"
				"					<attribute name='action'>app.diff-lx</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>L_y</attribute>"
				"					<attribute name='action'>app.diff-ly</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>L_xx</attribute>"
				"					<attribute name='action'>app.diff-lxx</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>L_xy</attribute>"
				"					<attribute name='action'>app.diff-lxy</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>L_yy</attribute>"
				"					<attribute name='action'>app.diff-lyy</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>L_w</attribute>"
				"					<attribute name='action'>app.diff-lw</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>L_vv</attribute>"
				"					<attribute name='action'>app.diff-lvv</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>L_vw</attribute>"
				"					<attribute name='action'>app.diff-lvw</attribute>"
				"				</item>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>L_ww</attribute>"
				"					<attribute name='action'>app.diff-lww</attribute>"
				"				</item>"
				"			</section>"
				"		</submenu>"
				"		<submenu>"
				"			<attribute name='label' translatable='yes'>_?</attribute>"
				"			<attribute name='action'>app.help-menu</attribute>"
				"			<section>"
				"				<item>"
				"					<attribute name='label' translatable='yes'>_About</attribute>"
				"					<attribute name='action'>app.about</attribute>"
				"				</item>"
				"			</section>"
				"		</submenu>"
				"	</menu>"
				"	<object class='GtkToolbar' id='ToolBar'>"
				"		<property name='visible'>True</property>"
				"		<child>"
				"			<object class='GtkToolButton' id='open-but'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"				<property name='tooltip_text' translatable='yes'>Open image</property>"
				"				<property name='action_name'>app.open-image</property>"
				"				<property name='icon_name'>document-open</property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkToolButton' id='save-but'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"				<property name='tooltip_text' translatable='yes'>Save image</property>"
				"				<property name='action_name'>app.save-image</property>"
				"				<property name='icon_name'>document-save</property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkSeparatorToolItem' id='tsep1'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkRadioToolButton' id='show-rgb'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"				<property name='tooltip_text' translatable='yes'>Show RGB image</property>"
				"				<property name='action_name'>app.show-image</property>"
				"				<property name='action_target'>0</property>"
				//"				<property name='icon_name'></property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkRadioToolButton' id='show-gray'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"				<property name='tooltip_text' translatable='yes'>Show Gray image</property>"
				"				<property name='action_name'>app.show-image</property>"
				"				<property name='action_target'>1</property>"
				//"				<property name='icon_name'></property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkRadioToolButton' id='show-bw'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"				<property name='tooltip_text' translatable='yes'>Show BW image</property>"
				"				<property name='action_name'>app.show-image</property>"
				"				<property name='action_target'>2</property>"
				//"				<property name='icon_name'></property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkRadioToolButton' id='show-other'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"				<property name='tooltip_text' translatable='yes'>Show result image</property>"
				"				<property name='action_name'>app.show-image</property>"
				"				<property name='action_target'>3</property>"
				//"				<property name='icon_name'></property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkSeparatorToolItem' id='tsep2'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkToolButton' id='zoomin-but'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"				<property name='tooltip_text' translatable='yes'>Zoom In</property>"
				"				<property name='action_name'>image.zoom-in</property>"
				"				<property name='icon_name'>zoom-in</property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkToolButton' id='zoomout-but'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"				<property name='tooltip_text' translatable='yes'>Zoom Out</property>"
				"				<property name='action_name'>image.zoom-out</property>"
				"				<property name='icon_name'>zoom-out</property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkToolButton' id='zoom100-but'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"				<property name='tooltip_text' translatable='yes'>Normal Size</property>"
				"				<property name='action_name'>image.zoom-100</property>"
				"				<property name='icon_name'>zoom-original</property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"		<child>"
				"			<object class='GtkToolButton' id='zoomfit-but'>"
				"				<property name='visible'>True</property>"
				"				<property name='can_focus'>False</property>"
				"				<property name='tooltip_text' translatable='yes'>Best Fit</property>"
				"				<property name='action_name'>image.zoom-fit</property>"
				"				<property name='icon_name'>zoom-fit-best</property>"
				"			</object>"
				"			<packing>"
				"				<property name='expand'>False</property>"
				"				<property name='homogeneous'>True</property>"
				"			</packing>"
				"		</child>"
				"	</object>"
				"</interface>";

			signal_realize().connect(sigc::bind(sigc::mem_fun(this, &Gtk::Widget::insert_action_group), "image", img.get_actions()));
			
			try
			{
				builder = Gtk::Builder::create_from_string(ui_info);
			}
			catch (Glib::Error &ex)
			{
				std::cout << ex.what() << '\n';
				return;
			}
#else
			// file menu
			actions->add(Gtk::Action::create("open-image", Gtk::Stock::OPEN, _("_Open image"), _("Open image")), sigc::mem_fun(this, &Titus::open_image));
			actions->add(Gtk::Action::create("save-image", Gtk::Stock::SAVE, _("_Save image"), _("Save image")), sigc::mem_fun(this, &Titus::save_image));

			// toolbar
			Gtk::RadioAction::Group g;
			actions->add(Gtk::RadioAction::create(g, "show-rgb", Gtk::StockID("gtk-crn-rgb"), _("_RGB"), _("Show RGB image")));
			Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("show-rgb"))->signal_toggled().connect(sigc::mem_fun(this, &Titus::on_image_toggled));
			actions->add(Gtk::RadioAction::create(g, "show-gray", Gtk::StockID("gtk-crn-gray"), _("_Gray"), _("Show Gray image")));
			Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("show-gray"))->signal_toggled().connect(sigc::mem_fun(this, &Titus::on_image_toggled));
			actions->add(Gtk::RadioAction::create(g, "show-bw", Gtk::StockID("gtk-crn-bw"), _("_BW"), _("Show BW image")));
			Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("show-bw"))->signal_toggled().connect(sigc::mem_fun(this, &Titus::on_image_toggled));
			actions->add(Gtk::RadioAction::create(g, "show-other", Gtk::StockID("gtk-crn-document"), _("_Result"), _("Show result image")));
			Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("show-other"))->signal_toggled().connect(sigc::mem_fun(this, &Titus::on_image_toggled));

			// generic menu
			actions->add(Gtk::Action::create("generic-menu", _("Gene_ric")));
			actions->add(Gtk::Action::create("generic-blur", _("_Gaussian blur")), sigc::mem_fun(this, &Titus::generic_blur));
			actions->add(Gtk::Action::create("generic-blur-x", _("Gaussian blur along _x")), sigc::mem_fun(this, &Titus::generic_blur_x));
			actions->add(Gtk::Action::create("generic-blur-y", _("Gaussian blur along _y")), sigc::mem_fun(this, &Titus::generic_blur_y));
			actions->add(Gtk::Action::create("generic-deriv-x", _("Gaussian _derivative along x")), sigc::mem_fun(this, &Titus::generic_deriv_x));
			actions->add(Gtk::Action::create("generic-deriv-y", _("Gaussian d_erivative along y")), sigc::mem_fun(this, &Titus::generic_deriv_y));
			actions->add(Gtk::Action::create("generic-2deriv-x", _("Gaussian _second derivative along x")), sigc::mem_fun(this, &Titus::generic_2deriv_x));
			actions->add(Gtk::Action::create("generic-2deriv-y", _("Gaussian se_cond derivative along y")), sigc::mem_fun(this, &Titus::generic_2deriv_y));

			// rgb menu
			actions->add(Gtk::Action::create("rgb-menu", _("_RGB")));
			actions->add(Gtk::Action::create("rgb-red", _("_Red")), sigc::mem_fun(this, &Titus::rgb_red));
			actions->add(Gtk::Action::create("rgb-green", _("_Green")), sigc::mem_fun(this, &Titus::rgb_green));
			actions->add(Gtk::Action::create("rgb-blue", _("_Blue")), sigc::mem_fun(this, &Titus::rgb_blue));
			actions->add(Gtk::Action::create("rgb-hue", _("_Hue")), sigc::mem_fun(this, &Titus::rgb_hue));
			actions->add(Gtk::Action::create("rgb-saturation", _("_Saturation")), sigc::mem_fun(this, &Titus::rgb_saturation));
			actions->add(Gtk::Action::create("rgb-pseudosaturation", _("Pseudo s_aturation")), sigc::mem_fun(this, &Titus::rgb_pseudosaturation));
			actions->add(Gtk::Action::create("rgb-value", _("_Value")), sigc::mem_fun(this, &Titus::rgb_value));
			actions->add(Gtk::Action::create("rgb-y", _("Luminance (_YUV)")), sigc::mem_fun(this, &Titus::rgb_y));
			actions->add(Gtk::Action::create("rgb-l", _("Luminance (HS_L)")), sigc::mem_fun(this, &Titus::rgb_l));
			actions->add(Gtk::Action::create("rgb-lprime", _("Pseudo l_uminance")), sigc::mem_fun(this, &Titus::rgb_lprime));

			actions->add(Gtk::Action::create("rgb-saturate", _("_Saturate")), sigc::mem_fun(this, &Titus::rgb_saturate));
			actions->add(Gtk::Action::create("rgb-edge-preserving-filter", _("Edge _preserving filter")), sigc::mem_fun(this, &Titus::rgb_epf));

			actions->add(Gtk::Action::create("rgb-diff", _("_Differential")), sigc::mem_fun(this, &Titus::rgb_diff));

			// gray menu
			actions->add(Gtk::Action::create("gray-menu", _("_Gray")));
			actions->add(Gtk::Action::create("gray-to-rgb", _("To _RGB")), sigc::mem_fun(this, &Titus::gray_to_rgb));
			actions->add(Gtk::Action::create("gray-threshold", _("_Threshold")), sigc::mem_fun(this, &Titus::gray_threshold));
			actions->add(Gtk::Action::create("gray-fisher", _("_Fisher")), sigc::mem_fun(this, &Titus::gray_fisher));
			actions->add(Gtk::Action::create("gray-entropy", _("_Entropy")), sigc::mem_fun(this, &Titus::gray_entropy));
			actions->add(Gtk::Action::create("gray-otsu", _("_Otsu")), sigc::mem_fun(this, &Titus::gray_otsu));
			actions->add(Gtk::Action::create("gray-niblack", _("_Niblack")), sigc::mem_fun(this, &Titus::gray_niblack));
			actions->add(Gtk::Action::create("gray-sauvola", _("_Sauvola")), sigc::mem_fun(this, &Titus::gray_sauvola));
			actions->add(Gtk::Action::create("gray-kmh", _("_k means histogram")), sigc::mem_fun(this, &Titus::gray_kmh));
			actions->add(Gtk::Action::create("gray-lmin", _("Local m_in")), sigc::mem_fun(this, &Titus::gray_lmin));
			actions->add(Gtk::Action::create("gray-lmax", _("Local m_ax")), sigc::mem_fun(this, &Titus::gray_lmax));

			actions->add(Gtk::Action::create("gray-strokes", _("_Strokes statistics")), sigc::mem_fun(this, &Titus::gray_strokes));
			actions->add(Gtk::Action::create("gray-histo", _("_Histogram")), sigc::mem_fun(this, &Titus::gray_histogram));
			actions->add(Gtk::Action::create("gray-rhisto", _("_Radial histogram")), sigc::mem_fun(this, &Titus::gray_rhistogram));

			actions->add(Gtk::Action::create("gray-diff", _("_Differential")), sigc::mem_fun(this, &Titus::gray_diff));

			// bw menu
			actions->add(Gtk::Action::create("bw-menu", _("_BW")));
			actions->add(Gtk::Action::create("bw-to-gray", _("To _gray")), sigc::mem_fun(this, &Titus::bw_to_gray));
			actions->add(Gtk::Action::create("bw-leftprof", _("_Left profile")), sigc::mem_fun(this, &Titus::bw_leftprof));
			actions->add(Gtk::Action::create("bw-rightprof", _("_Right profile")), sigc::mem_fun(this, &Titus::bw_rightprof));
			actions->add(Gtk::Action::create("bw-topprof", _("_Top profile")), sigc::mem_fun(this, &Titus::bw_topprof));
			actions->add(Gtk::Action::create("bw-bottomprof", _("_Bottom profile")), sigc::mem_fun(this, &Titus::bw_bottomprof));
			actions->add(Gtk::Action::create("bw-hproj", _("_Horizontal projection")), sigc::mem_fun(this, &Titus::bw_hproj));
			actions->add(Gtk::Action::create("bw-vproj", _("_Vertical projection")), sigc::mem_fun(this, &Titus::bw_vproj));

			// differential menu
			actions->add(Gtk::Action::create("diff-menu", _("_Differential")));
			actions->add(Gtk::Action::create("diff-diffuse", _("_Diffuse")), sigc::mem_fun(this, &Titus::diff_diffuse));
			actions->add(Gtk::Action::create("diff-gradgray", _("Gradient (_grayscale)")), sigc::mem_fun(this, &Titus::diff_gradgray));
			actions->add(Gtk::Action::create("diff-gradrgb", _("Gradient (_RGB)")), sigc::mem_fun(this, &Titus::diff_gradrgb));
			actions->add(Gtk::Action::create("diff-gradmod", _("Gradient _module")), sigc::mem_fun(this, &Titus::diff_gradmod));

			actions->add(Gtk::Action::create("diff-div", _("_Divergence")), sigc::mem_fun(this, &Titus::diff_div));
			actions->add(Gtk::Action::create("diff-laplacian", _("_Laplacian")), sigc::mem_fun(this, &Titus::diff_laplacian));
			actions->add(Gtk::Action::create("diff-edge", _("_Edge")), sigc::mem_fun(this, &Titus::diff_edge));
			actions->add(Gtk::Action::create("diff-corner", _("_Corner")), sigc::mem_fun(this, &Titus::diff_corner));
			actions->add(Gtk::Action::create("diff-k1", _("Kappa _1")), sigc::mem_fun(this, &Titus::diff_k1));
			actions->add(Gtk::Action::create("diff-k2", _("Kappa _2")), sigc::mem_fun(this, &Titus::diff_k2));
			actions->add(Gtk::Action::create("diff-hcorner", _("_Hessian corner")), sigc::mem_fun(this, &Titus::diff_hcorner));

			actions->add(Gtk::Action::create("diff-iso", _("_Isophote curvature")), sigc::mem_fun(this, &Titus::diff_iso));
			actions->add(Gtk::Action::create("diff-flow", _("_Flowline curvature")), sigc::mem_fun(this, &Titus::diff_flow));
			actions->add(Gtk::Action::create("diff-gaussc", _("_Gaussian curvature")), sigc::mem_fun(this, &Titus::diff_gaussc));
			actions->add(Gtk::Action::create("diff-gradc", _("G_radient curvature")), sigc::mem_fun(this, &Titus::diff_gradc));

			actions->add(Gtk::Action::create("diff-lx", _("L_x")), sigc::mem_fun(this, &Titus::diff_lx));
			actions->add(Gtk::Action::create("diff-ly", _("L_y")), sigc::mem_fun(this, &Titus::diff_ly));
			actions->add(Gtk::Action::create("diff-lxx", _("L_xx")), sigc::mem_fun(this, &Titus::diff_lxx));
			actions->add(Gtk::Action::create("diff-lxy", _("L_xy")), sigc::mem_fun(this, &Titus::diff_lxy));
			actions->add(Gtk::Action::create("diff-lyy", _("L_yy")), sigc::mem_fun(this, &Titus::diff_lyy));
			actions->add(Gtk::Action::create("diff-lw", _("L_w")), sigc::mem_fun(this, &Titus::diff_lw));
			actions->add(Gtk::Action::create("diff-lvv", _("L_vv")), sigc::mem_fun(this, &Titus::diff_lvv));
			actions->add(Gtk::Action::create("diff-lvw", _("L_vw")), sigc::mem_fun(this, &Titus::diff_lvw));
			actions->add(Gtk::Action::create("diff-lww", _("L_ww")), sigc::mem_fun(this, &Titus::diff_lww));

			ui_manager->insert_action_group(img.get_actions());
			add_accel_group(ui_manager->get_accel_group());

			Glib::ustring ui_info =
				"<ui>"
				"	<menubar name='MenuBar'>"
				"		<menu action='app-file-menu'>"
				"			<menuitem action='open-image'/>"
				"			<menuitem action='save-image'/>"
				"			<separator/>"
				"			<menuitem action='app-quit'/>"
				"		</menu>"
				"		<menu action='generic-menu'>"
				"			<menuitem action='generic-blur'/>"
				"			<menuitem action='generic-blur-x'/>"
				"			<menuitem action='generic-blur-y'/>"
				"			<separator/>"
				"			<menuitem action='generic-deriv-x'/>"
				"			<menuitem action='generic-deriv-y'/>"
				"			<menuitem action='generic-2deriv-x'/>"
				"			<menuitem action='generic-2deriv-y'/>"
				"		</menu>"
				"		<menu action='rgb-menu'>"
				"			<menuitem action='rgb-red'/>"
				"			<menuitem action='rgb-green'/>"
				"			<menuitem action='rgb-blue'/>"
				"			<menuitem action='rgb-hue'/>"
				"			<menuitem action='rgb-saturation'/>"
				"			<menuitem action='rgb-pseudosaturation'/>"
				"			<menuitem action='rgb-value'/>"
				"			<menuitem action='rgb-y'/>"
				"			<menuitem action='rgb-l'/>"
				"			<menuitem action='rgb-lprime'/>"
				"			<separator/>"
				"			<menuitem action='rgb-saturate'/>"
				"			<menuitem action='rgb-edge-preserving-filter'/>"
				"			<separator/>"
				"			<menuitem action='rgb-diff'/>"
				"		</menu>"
				"		<menu action='gray-menu'>"
				"			<menuitem action='gray-to-rgb'/>"
				"			<separator/>"
				"			<menuitem action='gray-threshold'/>"
				"			<menuitem action='gray-fisher'/>"
				"			<menuitem action='gray-entropy'/>"
				"			<menuitem action='gray-otsu'/>"
				"			<menuitem action='gray-niblack'/>"
				"			<menuitem action='gray-sauvola'/>"
				"			<menuitem action='gray-kmh'/>"
				"			<menuitem action='gray-lmin'/>"
				"			<menuitem action='gray-lmax'/>"
				"			<separator/>"
				"			<menuitem action='gray-strokes'/>"
				"			<menuitem action='gray-histo'/>"
				"			<menuitem action='gray-rhisto'/>"
				"			<separator/>"
				"			<menuitem action='gray-diff'/>"
				"		</menu>"
				"		<menu action='bw-menu'>"
				"			<menuitem action='bw-to-gray'/>"
				"			<separator/>"
				"			<menuitem action='bw-leftprof'/>"
				"			<menuitem action='bw-rightprof'/>"
				"			<menuitem action='bw-topprof'/>"
				"			<menuitem action='bw-bottomprof'/>"
				"			<menuitem action='bw-hproj'/>"
				"			<menuitem action='bw-vproj'/>"
				"		</menu>"
				"		<menu action='diff-menu'>"
				"			<menuitem action='diff-diffuse'/>"
				"			<separator/>"
				"			<menuitem action='diff-gradgray'/>"
				"			<menuitem action='diff-gradrgb'/>"
				"			<menuitem action='diff-gradmod'/>"
				"			<separator/>"
				"			<menuitem action='diff-div'/>"
				"			<menuitem action='diff-laplacian'/>"
				"			<menuitem action='diff-edge'/>"
				"			<menuitem action='diff-corner'/>"
				"			<menuitem action='diff-k1'/>"
				"			<menuitem action='diff-k2'/>"
				"			<menuitem action='diff-hcorner'/>"
				"			<separator/>"
				"			<menuitem action='diff-iso'/>"
				"			<menuitem action='diff-flow'/>"
				"			<menuitem action='diff-gaussc'/>"
				"			<menuitem action='diff-gradc'/>"
				"			<separator/>"
				"			<menuitem action='diff-lx'/>"
				"			<menuitem action='diff-ly'/>"
				"			<menuitem action='diff-lxx'/>"
				"			<menuitem action='diff-lxy'/>"
				"			<menuitem action='diff-lyy'/>"
				"			<menuitem action='diff-lw'/>"
				"			<menuitem action='diff-lvv'/>"
				"			<menuitem action='diff-lvw'/>"
				"			<menuitem action='diff-lww'/>"
				"		</menu>"
				"		<menu action='app-help-menu'>"
				"			<menuitem action='app-about'/>"
				"		</menu>"
				"	</menubar>"
				"	<toolbar name='ToolBar'>"
				"		<toolitem action='open-image'/>"
				"		<toolitem action='save-image'/>"
				"		<separator/>"
				"		<toolitem action='show-rgb'/>"
				"		<toolitem action='show-gray'/>"
				"		<toolitem action='show-bw'/>"
				"		<toolitem action='show-other'/>"
				"		<separator/>"
				"		<toolitem action='image-zoom-in'/>"
				"		<toolitem action='image-zoom-out'/>"
				"		<toolitem action='image-zoom-100'/>"
				"		<toolitem action='image-zoom-fit'/>"
				"	</toolbar>"
				"</ui>";
			ui_manager->add_ui_from_string(ui_info);
#endif
			Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox());
			vbox->show();
			add(*vbox);

#ifdef CRN_USING_GTKMM3
			auto menumodel = Glib::RefPtr<Gio::Menu>::cast_dynamic(builder->get_object("MenuBar"));
			auto *menu = new Gtk::MenuBar(menumodel);
			vbox->pack_start(*Gtk::manage(menu), false, true, 0);
			menu->show();
			Gtk::Toolbar *tb = nullptr;
			builder->get_widget("ToolBar", tb);
			vbox->pack_start(*tb, false, true, 0);
#else
			vbox->pack_start(*ui_manager->get_widget("/MenuBar"), false, true, 0);
			vbox->pack_start(*ui_manager->get_widget("/ToolBar"), false, true, 0);
#endif
			vbox->pack_start(img, true, true, 0);
			img.show();

			set_default_size(800, 600);

			fdial.set_modal();
			fdial.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
#ifdef CRN_USING_GTKMM3
			ff->add_pixbuf_formats();
#else
			ff.add_pixbuf_formats();
#endif
			fdial.set_filter(ff);
			fdial.add_button("ok", Gtk::RESPONSE_ACCEPT);
#ifdef CRN_USING_GTKMM3
			fdial.add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
			fdial.add_button(_("_Open"), Gtk::RESPONSE_ACCEPT);
#else
			fdial.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			fdial.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_ACCEPT);
			std::vector<int> altbut;
			altbut.push_back(Gtk::RESPONSE_ACCEPT);
			altbut.push_back(Gtk::RESPONSE_CANCEL);
			fdial.set_alternative_button_order_from_array(altbut);
#endif
			fdial.set_default_response(Gtk::RESPONSE_ACCEPT);

			//show_image(NONE);
			signal_realize().connect(sigc::bind(sigc::mem_fun(this, &Titus::show_image), NONE));
		}
		virtual ~Titus() override {}

	private:
		void open_image()
		{
			if (fdial.run() == Gtk::RESPONSE_ACCEPT)
			{
				fdial.hide();
				irgb.reset();
				igray.reset();
				ibw.reset();
				iother.reset();
				diff.reset();
				try
				{
					//Timer::Start();
					SImage cimg = NewImageFromFile(fdial.get_filename().c_str());
					//std::cout << "crn " << Timer::Stop() << std::endl;
					//Timer::Start();
					//Glib::RefPtr<Gdk::Pixbuf> pb(Gdk::Pixbuf::create_from_file(fdial.get_filename()));
					//std::cout << "gdk " << Timer::Stop() << std::endl;
					irgb = std::dynamic_pointer_cast<ImageRGB>(cimg);
					if (irgb)
					{
						show_image(RGB);
					}
					else
					{
						igray = std::dynamic_pointer_cast<ImageGray>(cimg);
						if (igray)
						{
							show_image(GRAY);
						}
						else
						{
							ibw = std::dynamic_pointer_cast<ImageBW>(cimg);
							if (ibw)
							{
								show_image(BW);
							}
							else
								throw 0;
						}
					}
				}
				catch (...)
				{
					show_message(_("Invalid image file."), Gtk::MESSAGE_ERROR);
					show_image(NONE);
				}
			}
			else
				fdial.hide();
		}

		void save_image()
		{
			static std::shared_ptr<Gtk::FileChooserDialog> dial(nullptr);
			if (!dial)
			{
				dial = std::make_shared<Gtk::FileChooserDialog>(*this, _("Select a PNG image to export…"), Gtk::FILE_CHOOSER_ACTION_SAVE);
				dial->set_modal();
				dial->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
#ifdef CRN_USING_GTKMM3
				auto ff = Gtk::FileFilter::create();
				ff->add_pixbuf_formats();
#else
				Gtk::FileFilter ff;
				ff.add_pixbuf_formats();
#endif
				dial->set_filter(ff);
#ifdef CRN_USING_GTKMM3
				dial->add_button(_("_Cancel"), Gtk::RESPONSE_CANCEL);
				dial->add_button(_("_Save"), Gtk::RESPONSE_ACCEPT);
#else
				dial->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
				dial->add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_ACCEPT);
				std::vector<int> altbut;
				altbut.push_back(Gtk::RESPONSE_ACCEPT);
				altbut.push_back(Gtk::RESPONSE_CANCEL);
				dial->set_alternative_button_order_from_array(altbut);
#endif
				dial->set_default_response(Gtk::RESPONSE_ACCEPT);
			}
			dial->set_current_name("export.png");
			int res = dial->run();
			dial->hide();
			if (res == Gtk::RESPONSE_ACCEPT)
			{
				currimg->SavePNG(dial->get_filename().c_str());
			}
		}

		void show_image(int mode)
		{
			current_image = mode;
			refreshing = true;
#ifdef CRN_USING_GTKMM3
			Gtk::RadioToolButton *tb = nullptr;
#endif
			switch (mode)
			{
				case NONE:
					currimg = nullptr;
					break;
				case RGB:
					currimg = irgb;
#ifdef CRN_USING_GTKMM3
					builder->get_widget("show-rgb", tb);
					tb->set_active();
#else
					Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("show-rgb"))->set_active();
#endif
					break;
				case GRAY:
					currimg = igray;
#ifdef CRN_USING_GTKMM3
					builder->get_widget("show-gray", tb);
					tb->set_active();
#else
					Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("show-gray"))->set_active();
#endif
					break;
				case BW:
					currimg = ibw;
#ifdef CRN_USING_GTKMM3
					builder->get_widget("show-bw", tb);
					tb->set_active();
#else
					Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("show-bw"))->set_active();
#endif
					break;
				default:
					currimg = iother;
#ifdef CRN_USING_GTKMM3
					builder->get_widget("show-other", tb);
					tb->set_active();
#else
					Glib::RefPtr<Gtk::RadioAction>::cast_dynamic(actions->get_action("show-other"))->set_active();
#endif
			}
			if (currimg)
				img.set_pixbuf(GdkCRN::PixbufFromCRNImage(*currimg));
			else
				img.set_pixbuf(Glib::RefPtr<Gdk::Pixbuf>(nullptr));
			refreshing = false;

			GtkCRN::set_enable_action(actions, "save-image", currimg != nullptr);
#ifdef CRN_USING_GTKMM3
			builder->get_widget("show-rgb", tb);
			tb->set_sensitive(irgb != nullptr);
			builder->get_widget("show-gray", tb);
			tb->set_sensitive(igray != nullptr);
			builder->get_widget("show-bw", tb);
			tb->set_sensitive(ibw != nullptr);
			builder->get_widget("show-other", tb);
			tb->set_sensitive(iother != nullptr);
#else
			GtkCRN::set_enable_action(actions, "show-rgb", irgb != nullptr);
			GtkCRN::set_enable_action(actions, "show-gray", igray != nullptr);
			GtkCRN::set_enable_action(actions, "show-bw", ibw != nullptr);
			GtkCRN::set_enable_action(actions, "show-other", iother != nullptr);
#endif
			GtkCRN::set_enable_action(actions, "generic-menu", mode != NONE);
			GtkCRN::set_enable_action(actions, "rgb-menu", irgb != nullptr);
			GtkCRN::set_enable_action(actions, "gray-menu", igray != nullptr);
			GtkCRN::set_enable_action(actions, "bw-menu", ibw != nullptr);
			GtkCRN::set_enable_action(actions, "diff-menu", diff != nullptr);

		}
#ifdef CRN_USING_GTKMM3
		void on_image_toggled(int id)
		{
			if (refreshing)
				return;
			auto obj = actions->lookup_action("show-image");
			auto act = Glib::RefPtr<Gio::SimpleAction>::cast_dynamic(obj);
			act->change_state(id);
			switch (id)
			{
				case 0:
					currimg = irgb;
					current_image = RGB;
					break;
				case 1:
					currimg = igray;
					current_image = GRAY;
					break;
				case 2:
					currimg = ibw;
					current_image = BW;
					break;
				case 3:
					currimg = iother;
					img.set_pixbuf(GdkCRN::PixbufFromCRNImage(*iother));
					current_image = OTHER;
					break;
				default:
					currimg = nullptr;
					current_image = NONE;
			}
			if (currimg)
				img.set_pixbuf(GdkCRN::PixbufFromCRNImage(*currimg));
			else
				img.set_pixbuf(Glib::RefPtr<Gdk::Pixbuf>(nullptr));
		}
#else
		void on_image_toggled()
		{
			auto &a = actions;
			if (!refreshing)
			{
				if (GtkCRN::is_toggle_action_active(a, "show-rgb"))
				{
					currimg = irgb;
					current_image = RGB;
				}
				else if (GtkCRN::is_toggle_action_active(a, "show-gray"))
				{
					currimg = igray;
					current_image = GRAY;
				}
				else if (GtkCRN::is_toggle_action_active(a, "show-bw"))
				{
					currimg = ibw;
					current_image = BW;
				}
				else if (GtkCRN::is_toggle_action_active(a, "show-other"))
				{
					currimg = iother;
					img.set_pixbuf(GdkCRN::PixbufFromCRNImage(*iother));
					current_image = OTHER;
				}
				else
				{
					currimg = nullptr;
					current_image = NONE;
				}
				if (currimg)
					img.set_pixbuf(GdkCRN::PixbufFromCRNImage(*currimg));
				else
					img.set_pixbuf(Glib::RefPtr<Gdk::Pixbuf>(nullptr));
			}
		}
#endif
		virtual void about() override
		{
			Gtk::AboutDialog dial;
			dial.set_transient_for ((Gtk::Window&)(*get_ancestor(GTK_TYPE_WINDOW)));
			dial.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
			dial.set_program_name("Titus");
			dial.set_version(CRN_PACKAGE_VERSION);
			dial.set_comments(_("Testing tool for libcrn functions"));
			dial.set_copyright("© CoReNum 2009-2014");
			dial.set_website(_("http://www.corenum.com/knowledge/libcrn.html"));
			dial.set_logo(GdkCRN::PixbufFromFile(Config::GetStaticDataPath() / "titus.png"));
			dial.show();
			dial.run();
		}
		////////////////////////////////////////////////////////////
		// generic
		////////////////////////////////////////////////////////////
		void generic_blur()
		{
			ParameterDialog dial(*this, _("Gaussian blur"));
			double sigma = 0.5;
			dial.add_value(_("Sigma"), &sigma, 0, 100);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				bool cont = false;
				try
				{
					auto& tmpi = dynamic_cast<ImageRGB&>(*currimg);
					tmpi.GaussianBlur(sigma);
				}
				catch (...) { cont = true; }
				if (cont)
				{
					cont = false;
					try
					{
						auto& tmpi = dynamic_cast<ImageGray&>(*currimg);
						tmpi.GaussianBlur(sigma);
					}
					catch (...) { cont = true; }
				}
				if (cont)
				{
					auto& tmpi = dynamic_cast<ImageBW&>(*currimg);
					tmpi.GaussianBlur(sigma);
				}
				show_image(current_image);
			}
		}
		void generic_blur_x()
		{
			ParameterDialog dial(*this, _("Gaussian blur along x"));
			double sigma = 0.5;
			dial.add_value(_("Sigma"), &sigma, 0, 100);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				MatrixDouble m = MatrixDouble::NewGaussianLine(sigma);
				bool cont = false;
				try
				{
					auto& tmpi = dynamic_cast<ImageRGB&>(*currimg);
					tmpi.Convolve(m);
				}
				catch (...) { cont = true; }
				if (cont)
				{
					cont = false;
					try
					{
						auto& tmpi = dynamic_cast<ImageGray&>(*currimg);
						tmpi.Convolve(m);
					}
					catch (...) { cont = true; }
				}
				if (cont)
				{
					auto& tmpi = dynamic_cast<ImageBW&>(*currimg);
					tmpi.Convolve(m);
				}
				show_image(current_image);
			}
		}
		void generic_blur_y()
		{
			ParameterDialog dial(*this, _("Gaussian blur along y"));
			double sigma = 0.5;
			dial.add_value(_("Sigma"), &sigma, 0, 100);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				MatrixDouble m = MatrixDouble::NewGaussianLine(sigma);
				m.Transpose();
				bool cont = false;
				try
				{
					auto& tmpi = dynamic_cast<ImageRGB&>(*currimg);
					tmpi.Convolve(m);
				}
				catch (...) { cont = true; }
				if (cont)
				{
					cont = false;
					try
					{
						auto& tmpi = dynamic_cast<ImageGray&>(*currimg);
						tmpi.Convolve(m);
					}
					catch (...) { cont = true; }
				}
				if (cont)
				{
					auto& tmpi = dynamic_cast<ImageBW&>(*currimg);
					tmpi.Convolve(m);
				}
				show_image(current_image);
			}
		}
		void generic_deriv_x()
		{
			ParameterDialog dial(*this, _("Gaussian derivative along x"));
			double sigma = 0.5;
			dial.add_value(_("Sigma"), &sigma, 0, 100);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				MatrixDouble m = MatrixDouble::NewGaussianLineDerivative(sigma);
				bool cont = false;
				try
				{
					auto tmpi = ImageIntRGB(dynamic_cast<ImageRGB&>(*currimg));
					tmpi.Convolve(m);
					iother = std::make_shared<ImageRGB>(Downgrade<ImageRGB>(tmpi));
				}
				catch (...) { cont = true; }
				if (cont)
				{
					cont = false;
					try
					{
						auto tmpi = ImageIntGray(dynamic_cast<ImageGray&>(*currimg));
						tmpi.Convolve(m);
						iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(tmpi));
					}
					catch (...) { cont = true; }
				}
				if (cont)
				{
					auto tmpi = ImageIntGray(dynamic_cast<ImageBW&>(*currimg));
					tmpi.Convolve(m);
					iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(tmpi));
				}
				show_image(OTHER);
			}
		}
		void generic_deriv_y()
		{
			ParameterDialog dial(*this, _("Gaussian derivative along y"));
			double sigma = 0.5;
			dial.add_value(_("Sigma"), &sigma, 0, 100);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				MatrixDouble m = MatrixDouble::NewGaussianLineDerivative(sigma);
				m.Transpose();
				bool cont = false;
				try
				{
					auto tmpi = ImageIntRGB(dynamic_cast<ImageRGB&>(*currimg));
					tmpi.Convolve(m);
					iother = std::make_shared<ImageRGB>(Downgrade<ImageRGB>(tmpi));
				}
				catch (...) { cont = true; }
				if (cont)
				{
					cont = false;
					try
					{
						auto tmpi = ImageIntGray(dynamic_cast<ImageGray&>(*currimg));
						tmpi.Convolve(m);
						iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(tmpi));
					}
					catch (...) { cont = true; }
				}
				if (cont)
				{
					auto tmpi = ImageIntGray(dynamic_cast<ImageBW&>(*currimg));
					tmpi.Convolve(m);
					iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(tmpi));
				}
				show_image(OTHER);
			}
		}
		void generic_2deriv_x()
		{
			ParameterDialog dial(*this, _("Gaussian second derivative along x"));
			double sigma = 0.5;
			dial.add_value(_("Sigma"), &sigma, 0, 100);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				MatrixDouble m = MatrixDouble::NewGaussianLineSecondDerivative(sigma);
				auto cont = false;
				try
				{
					auto tmpi = ImageIntRGB(dynamic_cast<ImageRGB&>(*currimg));
					tmpi.Convolve(m);
					iother = std::make_shared<ImageRGB>(Downgrade<ImageRGB>(tmpi));
				}
				catch (...) { cont = true; }
				if (cont)
				{
					cont = false;
					try
					{
						auto tmpi = ImageIntGray(dynamic_cast<ImageGray&>(*currimg));
						tmpi.Convolve(m);
						iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(tmpi));
					}
					catch (...) { cont = true; }
				}
				if (cont)
				{
					auto tmpi = ImageIntGray(dynamic_cast<ImageBW&>(*currimg));
					tmpi.Convolve(m);
					iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(tmpi));
				}
				show_image(OTHER);
			}
		}
		void generic_2deriv_y()
		{
			ParameterDialog dial(*this, _("Gaussian second derivative along y"));
			double sigma = 0.5;
			dial.add_value(_("Sigma"), &sigma, 0, 100);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				MatrixDouble m = MatrixDouble::NewGaussianLineSecondDerivative(sigma);
				m.Transpose();
				auto cont = false;
				try
				{
					auto tmpi = ImageIntRGB(dynamic_cast<ImageRGB&>(*currimg));
					tmpi.Convolve(m);
					iother = std::make_shared<ImageRGB>(Downgrade<ImageRGB>(tmpi));
				}
				catch (...) { cont = true; }
				if (cont)
				{
					cont = false;
					try
					{
						auto tmpi = ImageIntGray(dynamic_cast<ImageGray&>(*currimg));
						tmpi.Convolve(m);
						iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(tmpi));
					}
					catch (...) { cont = true; }
				}
				if (cont)
				{
					auto tmpi = ImageIntGray(dynamic_cast<ImageBW&>(*currimg));
					tmpi.Convolve(m);
					iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(tmpi));
				}
				show_image(OTHER);
			}
		}
		////////////////////////////////////////////////////////////
		// RGB
		////////////////////////////////////////////////////////////
		void rgb_red()
		{
			igray = std::make_shared<ImageGray>(RedChannel(*irgb));
			show_image(GRAY);
		}
		void rgb_green()
		{
			igray = std::make_shared<ImageGray>(GreenChannel(*irgb));
			show_image(GRAY);
		}
		void rgb_blue()
		{
			igray = std::make_shared<ImageGray>(BlueChannel(*irgb));
			show_image(GRAY);
		}
		void rgb_hue()
		{
			igray = std::make_shared<ImageGray>(HChannel(ImageHSV(*irgb)));
			show_image(GRAY);
		}
		void rgb_saturation()
		{
			igray = std::make_shared<ImageGray>(SChannel(*irgb));
			show_image(GRAY);
		}
		void rgb_pseudosaturation()
		{
			igray = std::make_shared<ImageGray>(PseudoSaturation(*irgb));
			show_image(GRAY);
		}
		void rgb_value()
		{
			igray = std::make_shared<ImageGray>(VChannel(*irgb));
			show_image(GRAY);
		}
		void rgb_y()
		{
			igray = std::make_shared<ImageGray>(YChannel(*irgb));
			show_image(GRAY);
		}
		void rgb_l()
		{
			igray = std::make_shared<ImageGray>(LChannel(*irgb));
			show_image(GRAY);
		}
		void rgb_lprime()
		{
			igray = std::make_shared<ImageGray>(LPrime(*irgb));
			show_image(GRAY);
		}
		void rgb_saturate()
		{
			irgb = std::make_shared<ImageRGB>(ColorWheel(HChannel(ImageHSV(*irgb))));
			show_image(RGB);
		}
		void rgb_epf()
		{
			ParameterDialog dial(*this, _("Edge preserving filter"));
			int iter = 5;
			dial.add_value(_("# iterations"), &iter, 1, 100);
			int maxval = 30;
			dial.add_value(_("Maximal distance between colors of a same class"), &maxval, 1, 255);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				EdgePreservingFilter(*irgb, iter, uint8_t(maxval));
				show_image(RGB);
			}
		}
		void rgb_diff()
		{
			ParameterDialog dial(*this, _("Differential"));
			double sigma = 0.5;
			dial.add_value(_("Sigma"), &sigma, 0, 100);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				diff = std::make_unique<Differential>(Differential::NewGaussian(*irgb, Differential::RGBProjection::ABSMAX, sigma));
				GtkCRN::enable_action(actions, "diff-menu");
			}
		}
		////////////////////////////////////////////////////////////
		// Gray
		////////////////////////////////////////////////////////////
		void gray_to_rgb()
		{
			irgb = std::make_shared<ImageRGB>(*igray);
			show_image(RGB);
		}

		void gray_threshold()
		{
			ParameterDialog dial(*this, _("Threshold"));
			int t = 127;
			dial.add_value(_("Threshold"), &t, 0, 255);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				ibw = std::make_shared<ImageBW>(Threshold(*igray, uint8_t(t)));
				show_image(BW);
			}
		}
		void gray_fisher()
		{
			ibw = std::make_shared<ImageBW>(Fisher(*igray));
			show_image(BW);
		}
		void gray_entropy()
		{
			ibw = std::make_shared<ImageBW>(Entropy(*igray));
			show_image(BW);
		}
		void gray_otsu()
		{
			ibw = std::make_shared<ImageBW>(Otsu(*igray));
			show_image(BW);
		}
		void gray_niblack()
		{
			ParameterDialog dial(*this, _("Niblack"));
			int w = 3;
			dial.add_value(_("Half window"), &w, 1, int(igray->GetWidth()) / 20);
			double k = 0.5;
			dial.add_value(_("k"), &k, 0, 5);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				ibw = std::make_shared<ImageBW>(Niblack(*igray, w, k));
				show_image(BW);
			}
		}
		void gray_sauvola()
		{
			ParameterDialog dial(*this, _("Sauvola"));
			int w = 3;
			dial.add_value(_("Half window"), &w, 1, int(igray->GetWidth()) / 20);
			double k = 0.5;
			dial.add_value(_("k"), &k, 0, 5);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				ibw = std::make_shared<ImageBW>(Sauvola(*igray, w, k));
				show_image(BW);
			}
		}
		void gray_kmh()
		{
			ParameterDialog dial(*this, _("k means histogram"));
			int c = 5;
			dial.add_value(_("Number of classes"), &c, 2, 100);
			int k = 3;
			dial.add_value(_("Number of black classes"), &k, 1, 99);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				if (c > k)
				{
					ibw = std::make_shared<ImageBW>(kMeansHisto(*igray, c, k));
					show_image(BW);
				}
				else
				{
					show_message(_("The number of black classes must be lower than the total number of classes."), Gtk::MESSAGE_ERROR);
				}
			}
		}
		void gray_lmin()
		{
			ParameterDialog dial(*this, _("Local min"));
			int r = 1;
			dial.add_value(_("Radius"), &r, 1, int(igray->GetWidth()) / 20);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				ibw = std::make_shared<ImageBW>(LocalMin(*igray, r));
				show_image(BW);
			}
		}
		void gray_lmax()
		{
			ParameterDialog dial(*this, _("Local max"));
			int r = 1;
			dial.add_value(_("Radius"), &r, 1, int(igray->GetWidth()) / 20);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				ibw = std::make_shared<ImageBW>(LocalMax(*igray, r));
				show_image(BW);
			}
		}
		void gray_strokes()
		{
			size_t sw = StrokesWidth(*igray);
			size_t sh = StrokesHeight(*igray);
			StringUTF8 msg = _("Strokes width: ");
			msg += sw;
			msg += "\n";
			msg += _("Strokes height: ");
			msg += sh;
			show_message(msg.CStr(), Gtk::MESSAGE_INFO);
		}
		void gray_histogram()
		{
			Histogram hi(MakeHistogram(*igray));
			iother = std::make_shared<ImageBW>(hi.MakeImageBW(256));
			show_image(OTHER);
		}
		void gray_rhistogram()
		{
			Histogram hi(MakeHistogram(*igray));
			iother = std::make_shared<ImageBW>(hi.MakeRadialImageBW(256));
			show_image(OTHER);
		}
		void gray_diff()
		{
			ParameterDialog dial(*this, _("Differential"));
			double sigma = 0.5;
			dial.add_value(_("Sigma"), &sigma, 0, 100);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				diff = std::make_unique<Differential>(Differential::NewGaussian(*igray, sigma));
				GtkCRN::enable_action(actions, "diff-menu");
			}
		}
		////////////////////////////////////////////////////////////
		// BW
		////////////////////////////////////////////////////////////
		void bw_to_gray()
		{
			igray = std::make_shared<ImageGray>(*ibw);
			show_image(GRAY);
		}
		void bw_leftprof()
		{
			Histogram h(LeftProfile(*ibw));
			iother = std::make_shared<ImageBW>(MakeRotation(h.MakeImageBW(h.Max()), 270, pixel::BWWhite));
			show_image(OTHER);
		}
		void bw_rightprof()
		{
			Histogram h(RightProfile(*ibw));
			iother  = std::make_shared<ImageBW>(MakeRotation(h.MakeImageBW(h.Max()), 90, pixel::BWWhite));
			show_image(OTHER);
		}
		void bw_topprof()
		{
			Histogram h(TopProfile(*ibw));
			iother = std::make_shared<ImageBW>(MakeRotation(h.MakeImageBW(h.Max()), 180, pixel::BWWhite));
			show_image(OTHER);
		}
		void bw_bottomprof()
		{
			Histogram h(TopProfile(*ibw));
			iother = std::make_shared<ImageBW>(h.MakeImageBW(h.Max()));
			show_image(OTHER);
		}
		void bw_hproj()
		{
			Histogram h(HorizontalProjection(*ibw));
			iother = std::make_shared<ImageBW>(MakeRotation(h.MakeImageBW(h.Max()), 270, pixel::BWWhite));
			show_image(OTHER);
		}
		void bw_vproj()
		{
			Histogram h(VerticalProjection(*ibw));
			iother = std::make_shared<ImageBW>(h.MakeImageBW(h.Max()));
			show_image(OTHER);
		}
		////////////////////////////////////////////////////////////
		// BW
		////////////////////////////////////////////////////////////
		void diff_diffuse()
		{
			ParameterDialog dial(*this, _("Differential"));
			int iter = 1;
			dial.add_value(_("Number of iterations"), &iter, 1, 100);
			double maxdiv = 3.0;
			dial.add_value(_("Max divergence to allow modification of a pixel"), &maxdiv, 0.00001, 3.0);
			if (dial.run() == Gtk::RESPONSE_ACCEPT)
			{
				diff->Diffuse(iter, maxdiv);
			}
		}
		void diff_gradgray()
		{
			iother = std::make_shared<ImageGray>(diff->MakeImageGradient().MakeImageGray());
			show_image(OTHER);
		}
		void diff_gradrgb()
		{
			iother = std::make_shared<ImageRGB>(diff->MakeImageGradient().MakeImageRGB());
			show_image(OTHER);
		}
		void diff_gradmod()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeGradientModule()));
			show_image(OTHER);
		}
		void diff_div()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeDivergence()));
			show_image(OTHER);
		}
		void diff_laplacian()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeLaplacian()));
			show_image(OTHER);
		}
		void diff_edge()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeEdge()));
			show_image(OTHER);
		}
		void diff_corner()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeCorner()));
			show_image(OTHER);
		}
		void diff_k1()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeKappa1()));
			show_image(OTHER);
		}
		void diff_k2()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeKappa2()));
			show_image(OTHER);
		}
		void diff_hcorner()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeHessianCorner()));
			show_image(OTHER);
		}
		void diff_iso()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeIsophoteCurvature()));
			show_image(OTHER);
		}
		void diff_flow()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeFlowlineCurvature()));
			show_image(OTHER);
		}
		void diff_gaussc()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeGaussianCurvature()));
			show_image(OTHER);
		}
		void diff_gradc()
		{
			iother = std::make_shared<ImageGray>(diff->MakeGradientCurvature());
			show_image(OTHER);
		}
		void diff_lx()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->GetLx()));
			show_image(OTHER);
		}
		void diff_ly()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->GetLy()));
			show_image(OTHER);
		}
		void diff_lxx()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->GetLxx()));
			show_image(OTHER);
		}
		void diff_lxy()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->GetLxy()));
			show_image(OTHER);
		}
		void diff_lyy()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->GetLyy()));
			show_image(OTHER);
		}
		void diff_lw()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeLw()));
			show_image(OTHER);
		}
		void diff_lvv()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeLvv()));
			show_image(OTHER);
		}
		void diff_lvw()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeLvw()));
			show_image(OTHER);
		}
		void diff_lww()
		{
			iother = std::make_shared<ImageGray>(Downgrade<ImageGray>(diff->MakeLww()));
			show_image(OTHER);
		}

		////////////////////////////////////////////////////////////
		// inner classes and attributes
		////////////////////////////////////////////////////////////
		enum ImageType { NONE, RGB, GRAY, BW, OTHER };

		class ParameterDialog: public Gtk::Dialog
	{
		public:
			ParameterDialog(Gtk::Window &parent, const Glib::ustring &name):
				Gtk::Dialog(name, parent, true)
		{
			set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
#ifdef CRN_USING_GTKMM3
			add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
			add_button(_("OK"), Gtk::RESPONSE_ACCEPT);
#else
			add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
			add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
			std::vector<int> altbut;
			altbut.push_back(Gtk::RESPONSE_ACCEPT);
			altbut.push_back(Gtk::RESPONSE_CANCEL);
			set_alternative_button_order_from_array(altbut);
#endif
			set_default_response(Gtk::RESPONSE_ACCEPT);
			signal_response().connect(sigc::mem_fun(this, &ParameterDialog::set_values));

			set_default_size(500, -1);
		}
			virtual ~ParameterDialog() override {}
			void add_value(const Glib::ustring &name, int *val, int min, int max)
			{
				Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox);
				hbox->pack_start(*Gtk::manage(new Gtk::Label(name)), false, true, 2);
#ifdef CRN_USING_GTKMM3
				Gtk::Scale *scale = Gtk::manage(new Gtk::Scale);
#else
				Gtk::HScale *scale = Gtk::manage(new Gtk::HScale);
#endif
				integers[scale] = val;
				scale->set_range(min, max);
				scale->set_increments(1, 1);
				scale->set_digits(0);
				scale->set_draw_value(true);
				scale->set_value_pos(Gtk::POS_LEFT);
				scale->set_value(*val);
				hbox->pack_start(*scale, true, true, 2);
				hbox->show_all();
				get_content_area()->pack_start(*hbox, false, false, 2);
			}
			void add_value(const Glib::ustring &name, double *val, double min, double max)
			{
				Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox);
				hbox->pack_start(*Gtk::manage(new Gtk::Label(name)), false, true, 2);
				Gtk::SpinButton *spin = Gtk::manage(new Gtk::SpinButton);
				reals[spin] = val;
				spin->set_digits(2);
				spin->set_range(min, max);
				spin->set_increments(1, 1);
				spin->set_value(*val);
				hbox->pack_start(*spin, true, true, 2);
				hbox->show_all();
				get_content_area()->pack_start(*hbox, false, false, 2);
			}
			void add_value(const Glib::ustring &name, bool *val)
			{
				Gtk::CheckButton *check = Gtk::manage(new Gtk::CheckButton(name));
				booleans[check] = val;
				check->set_active(*val);
				check->show();
				get_content_area()->pack_start(*check, true, false, 2);
			}

		private:
			void set_values(int resp)
			{
				if (resp == Gtk::RESPONSE_ACCEPT)
				{
					for (std::map<Gtk::Scale*, int*>::iterator it = integers.begin(); it != integers.end(); ++it)
					{
						*it->second = int(it->first->get_value());
					}
					for (std::map<Gtk::SpinButton*, double*>::iterator it = reals.begin(); it != reals.end(); ++it)
					{
						*it->second = it->first->get_value();
					}
					for (std::map<Gtk::CheckButton*, bool*>::iterator it = booleans.begin(); it != booleans.end(); ++it)
					{
						*it->second = it->first->get_active();
					}
				}
			}
			std::map<Gtk::Scale*, int*> integers;
			std::map<Gtk::SpinButton*, double*> reals;
			std::map<Gtk::CheckButton*, bool*> booleans;
	};

#ifdef CRN_USING_GTKMM3
		Glib::RefPtr<Gtk::Builder> builder;
#endif
		GtkCRN::Image img;
		int current_image;
		Gtk::FileChooserDialog fdial;
#ifdef CRN_USING_GTKMM3
		Glib::RefPtr<Gtk::FileFilter> ff;
#else
		Gtk::FileFilter ff;
#endif
		bool refreshing;

		SImageRGB irgb;
		SImageGray igray;
		SImageBW ibw;
		SImage iother;
		UDifferential diff;
		SImage currimg;
};

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	CRNbindtextdomain(GETTEXT_PACKAGE, CRN_LOCALE_FULL_PATH);
	CRNbind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	CRNtextdomain(GETTEXT_PACKAGE);

#ifdef CRN_USING_GTKMM3
	auto gapp =
		Gtk::Application::create(argc, argv,
				"org.libcrn.titus");
#else
	GtkCRN::Main kit(argc, argv);
	GtkCRN::Main::SetDefaultExceptionHandler();
#endif
	Titus app;
	GtkCRN::App::set_main_window(&app);
#ifdef CRN_USING_GTKMM3
	return gapp->run(app);
#else
	app.show();

	kit.run_thread_safe();
	return 0;
#endif
}

