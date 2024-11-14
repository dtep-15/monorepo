use leptos::prelude::*;

pub fn push_right() -> impl IntoView {
	view! {
		<span class="push-right"></span>
	}
}

pub fn loading() -> impl IntoView {
	view! {
		<dt-centered-col>
			<h1>Loading...</h1>
			<dt-loading-bar></dt-loading-bar>
		</dt-centered-col>
	}
}


// pub fn local_storage() -> Option<Storage> {
// 	window().local_storage().ok().flatten()
// }
