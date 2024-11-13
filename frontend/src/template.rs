use leptos::prelude::*;

#[component]
pub fn template(children: Children) -> impl IntoView {
	view! {
		<dt-app>
			{children()}
		</dt-app>
	}
}
